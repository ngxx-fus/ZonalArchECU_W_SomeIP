## @file MonitorMain.py
## @brief Main entry point for the Zonal Architecture Monitor application.

import sys
import socket
from datetime import datetime
from PySide6.QtWidgets import QApplication, QMainWindow, QVBoxLayout, QTextEdit
from PySide6.QtCore import Qt

from matplotlib.backends.backend_qtagg import FigureCanvasQTAgg
from matplotlib.figure import Figure

from MonitorConfig import *

# /* Import external components */
from monitor_ui import Ui_MainWindow
from CoreNetwork import ZoneECU, CCUCommandBuilder
from UdpListener import UdpListenerThread

# /* ========================================================================= */
# /* MAIN GUI APPLICATION                                                      */
# /* ========================================================================= */
class MyMonitorApp(QMainWindow):
    # /*
    #  * @brief Constructor for the Main GUI Application.
    #  */
    def __init__(self):
        super().__init__()
        SysLog("Initializing MyMonitorApp constructor.")
        
        self.ui = Ui_MainWindow()
        self.ui.setupUi(self)
        
        # /* Core Registries */
        self.ecu_registry = {}
        
        # /* ZoneECU Classification Array */
        self.zone_ecus = {
            "Front": None,
            "Back": None
        }
        
        # /* Local State tracking for Command Generation */
        self.sys_engine_started = False
        self.motor_states = {
            "Front": {"L": 0, "R": 0},
            "Back": {"L": 0, "R": 0}
        }
        
        self.global_packet_count = 0
        self.update_interval = 2 

        self.setup_graph()
        self.setup_logging_console()
        self.connect_signals()
        
        self.udp_thread = UdpListenerThread()
        self.udp_thread.data_received.connect(self.process_incoming_data)
        self.udp_thread.start()

        self.app_log("System Ready. Waiting for ECU Heartbeats...")
        SysInfo("System Ready. Waiting for ECU Heartbeats...")

    # /*
    #  * @brief Upgrades the static QLabel into a dynamic QTextEdit for logging.
    #  */
    def setup_logging_console(self):
        SysLog("Upgrading static QLabel to dynamic QTextEdit for logging.")
        geom = self.ui.label.geometry()
        parent = self.ui.label.parentWidget()
        
        self.ui.label.deleteLater()
        
        self.log_console = QTextEdit(parent)
        self.log_console.setGeometry(geom)
        self.log_console.setReadOnly(True)
        self.log_console.document().setMaximumBlockCount(20) 
        
        # /* Return from execution */
        return

    # /*
    #  * @brief Binds all UI events to their respective callback functions.
    #  */
    def connect_signals(self):
        SysLog("Binding UI events to callback functions.")
        self.ui.Control_StartStop.clicked.connect(self.on_start_stop_clicked)
        self.ui.Control_MoveForward.clicked.connect(self.on_forward_clicked)
        self.ui.Control_MoveBackward.clicked.connect(self.on_backward_clicked)
        
        self.ui.Control_Front_Motor_Control_Slider_Left.valueChanged.connect(self.on_slider_front_left_changed)
        self.ui.Control_Front_Motor_Control_Slider_Right.valueChanged.connect(self.on_slider_front_right_changed)
        self.ui.Control_Back_Motor_Control_Slider_Left.valueChanged.connect(self.on_slider_back_left_changed)
        self.ui.Control_Back_Motor_Control_Slider_Right.valueChanged.connect(self.on_slider_back_right_changed)
        
        self.ui.actionClose_app.triggered.connect(self.close)
        
        # /* Return from execution */
        return

    # /*
    #  * @brief Appends a timestamped message to the UI log console.
    #  * @param message The string content to display.
    #  */
    def app_log(self, message):
        now = datetime.now().strftime("[%H:%M:%S] ")
        self.log_console.append(now + message)
        
        # /* Return from execution */
        return

    # /*
    #  * @brief Transmits a byte array payload back to a specific ZoneECU.
    #  * @param target_ecu The ZoneECU object destination.
    #  * @param data The raw byte array to transmit.
    #  */
    def UDPSendBack(self, target_ecu, data):
        SysLog("Preparing to transmit UDP data to %s.", target_ecu.name if target_ecu else "Unknown")
        
        # /* Guard clause to ensure valid destination */
        if target_ecu is None or not target_ecu.ip:
            self.app_log("TX ERROR: Target ECU is undefined or missing IP.")
            SysErr("TX ERROR: Target ECU is undefined or missing IP.")
            return
            
        try:
            self.udp_thread.sock.sendto(data, (target_ecu.ip, target_ecu.port))
            # sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
            # sock.sendto(data, (target_ecu.ip, target_ecu.port))
            # sock.close()
            self.app_log(f"TX -> {target_ecu.name} [{target_ecu.ip}:{target_ecu.port}] | Size: {len(data)} bytes")
            SysInfo("TX -> %s [%s:%d] | Size: %d bytes", target_ecu.name, target_ecu.ip, target_ecu.port, len(data))
        except Exception as e:
            self.app_log(f"TX ERROR: Failed to transmit to {target_ecu.name}. Reason: {e}")
            SysErr("TX ERROR: Failed to transmit to %s. Reason: %s", target_ecu.name, str(e))
            
        # /* Return from execution */
        return

    # /*
    #  * @brief Initializes the embedded Matplotlib charting environment.
    #  */
    def setup_graph(self):
        SysLog("Initializing embedded Matplotlib charting environment.")
        self.fig = Figure(figsize=(5, 4), dpi=100)
        self.fig.tight_layout()
        
        self.ax1 = self.fig.add_subplot(311)
        self.ax1.set_ylabel('Sync (0-3)')
        self.ax1.grid(True)
        
        self.ax2 = self.fig.add_subplot(312, sharex=self.ax1)
        self.ax2.set_ylabel('Dist (cm)')
        self.ax2.grid(True)
        
        self.ax3 = self.fig.add_subplot(313, sharex=self.ax1)
        self.ax3.set_xlabel('Packet Index')
        self.ax3.set_ylabel('Motor (RPM)')
        self.ax3.grid(True)
        
        self.canvas = FigureCanvasQTAgg(self.fig)
        layout = QVBoxLayout(self.ui.Graph)
        layout.setContentsMargins(0, 0, 0, 0)
        layout.addWidget(self.canvas)
        
        # /* Return from execution */
        return

    # /*
    #  * @brief Registers a newly discovered ECU and initializes its plotting data.
    #  * @param ecu_name The string identifier of the ECU.
    #  */
    def register_new_ecu(self, ecu_name):
        SysInfo("Registering newly discovered ECU: %s", ecu_name)
        stats = {
            "received": 0, "last_seq": -1, "last_time": 0.0,
            "dist_sync_history": [], "motor_sync_history": [],
            "d0_history": [], "d1_history": [], "d2_history": [],
            "m0_history": [], "m1_history": []
        }
        
        lines = {
            "dist_seq": self.ax1.plot([], [], marker='o', markersize=3, label=f'[{ecu_name}] Dist Sync')[0],
            "d0": self.ax2.plot([], [], marker='o', markersize=3, label=f'[{ecu_name}] D0')[0],
            "d1": self.ax2.plot([], [], marker='x', markersize=3, label=f'[{ecu_name}] D1')[0],
            "d2": self.ax2.plot([], [], marker='^', markersize=3, label=f'[{ecu_name}] D2')[0],
            "m0": self.ax3.plot([], [], marker='o', markersize=3, label=f'[{ecu_name}] M0')[0],
            "m1": self.ax3.plot([], [], marker='x', markersize=3, label=f'[{ecu_name}] M1')[0],
        }
        
        self.ax1.legend(loc='upper left', fontsize='x-small')
        self.ax2.legend(loc='upper left', fontsize='x-small')
        self.ax3.legend(loc='upper left', fontsize='x-small')
        
        self.ecu_registry[ecu_name] = { "stats": stats, "lines": lines }
        
        # /* Return from execution */
        return

    # /*
    #  * @brief Processes data emitted from the UDP listening thread.
    #  * @param payload Dictionary containing extracted payload attributes.
    #  */
    def process_incoming_data(self, payload):
        self.global_packet_count += 1
        ecu_name = payload["name"]
        
        SysLog("Processing incoming packet from ECU: %s", ecu_name)
        
        # /* Evaluate ECU origin to classify as Front or Back */
        ecu_type = None
        if "Front" in ecu_name:
            ecu_type = "Front"
        elif "Back" in ecu_name or "Rear" in ecu_name:
            ecu_type = "Back"
            
        # /* Conditional steering to validate ECU assignment */
        if ecu_type:
            existing_ecu = self.zone_ecus[ecu_type]
            
            # /* Branch if ECU slot is currently empty */
            if existing_ecu is None:
                self.zone_ecus[ecu_type] = ZoneECU(ecu_name, payload["ip"], payload["port"], payload["mac"])
                self.app_log(f"REGISTERED: {ecu_type} Zone -> {ecu_name} [{payload['ip']}:{payload['port']} | MAC: {payload['mac']}]")
                SysInfo("REGISTERED: %s Zone -> %s [%s:%d | MAC: %s]", ecu_type, ecu_name, payload["ip"], payload["port"], payload["mac"])
            # /* Branch if conflicting ECU is detected in the same slot */
            elif existing_ecu.name != ecu_name or existing_ecu.ip != payload["ip"]:
                self.app_log(f"ERROR: Conflict in {ecu_type} Zone! Existing: {existing_ecu.name}, Incoming: {ecu_name}")
                SysErr("Conflict in %s Zone! Existing: %s, Incoming: %s", ecu_type, existing_ecu.name, ecu_name)
                # /* Return from execution to discard conflicting packet */
                return
        else:
            self.app_log(f"ERROR: Unknown ECU classification for {ecu_name}")
            SysErr("Unknown ECU classification for %s", ecu_name)
            # /* Return from execution */
            return
        
        # /* Conditional steering to register previously unseen ECUs for plotting */
        if ecu_name not in self.ecu_registry:
            self.register_new_ecu(ecu_name)
            
        stats = self.ecu_registry[ecu_name]["stats"]
        stats["received"] += 1
        
        stats["dist_sync_history"].append(payload["dist_sync"])
        stats["motor_sync_history"].append(payload["motor_sync"])
        stats["d0_history"].append(payload["d0"])
        stats["d1_history"].append(payload["d1"])
        stats["d2_history"].append(payload["d2"])
        stats["m0_history"].append(payload["m0"])
        stats["m1_history"].append(payload["m1"])
        
        max_points = 50
        
        # /* Loop to truncate historical arrays, preventing memory overflow */
        for key in stats:
            # /* Conditional branching to verify data type and length */
            if isinstance(stats[key], list) and len(stats[key]) > max_points:
                stats[key] = stats[key][-max_points:]

        self.update_gui_elements(ecu_name, payload["d0"], payload["d1"], payload["d2"], payload["m0"], payload["m1"])

        # /* Trigger graph rendering on defined intervals */
        if self.global_packet_count % self.update_interval == 0:
            self.refresh_graph(max_points)
            
        # /* Return from execution */
        return

    # /*
    #  * @brief Updates numeric labels and sliders based on telemetry.
    #  */
    def update_gui_elements(self, ecu_name, d0, d1, d2, m0, m1):
        SysLog("Updating GUI elements for ECU: %s", ecu_name)
        f_d0 = f"{d0/100:.2f}m"
        f_d1 = f"{d1/100:.2f}m"
        f_d2 = f"{d2/100:.2f}m"
        
        # /* Evaluate ECU origin to route data to the appropriate UI cluster */
        if "Front" in ecu_name:
            self.ui.Visualization_Front_Distance_Sensor_Left.setText(f_d0)
            self.ui.Visualization_Front_Distance_Sensor_Center.setText(f_d1)
            self.ui.Visualization_Front_Distance_Sensor_Right.setText(f_d2)
            
            self.ui.Control_Front_Motor_Control_Slider_Left.blockSignals(True)
            self.ui.Control_Front_Motor_Control_Slider_Right.blockSignals(True)
            
            self.ui.Control_Front_Motor_Control_Slider_Left.setValue(m0)
            self.ui.Control_Front_Motor_Control_Slider_Right.setValue(m1)
            
            self.ui.Control_Front_Motor_Control_Slider_Left.blockSignals(False)
            self.ui.Control_Front_Motor_Control_Slider_Right.blockSignals(False)
            
        elif "Back" in ecu_name or "Rear" in ecu_name:
            self.ui.Visualization_Back_Distance_Sensor_Left.setText(f_d0)
            self.ui.Visualization_Back_Distance_Sensor_Center.setText(f_d1)
            self.ui.Visualization_Back_Distance_Sensor_Right.setText(f_d2)
            
            self.ui.Control_Back_Motor_Control_Slider_Left.blockSignals(True)
            self.ui.Control_Back_Motor_Control_Slider_Right.blockSignals(True)
            
            self.ui.Control_Back_Motor_Control_Slider_Left.setValue(m0)
            self.ui.Control_Back_Motor_Control_Slider_Right.setValue(m1)
            
            self.ui.Control_Back_Motor_Control_Slider_Left.blockSignals(False)
            self.ui.Control_Back_Motor_Control_Slider_Right.blockSignals(False)
            
        # /* Return from execution */
        return

    # /*
    #  * @brief Redraws the Matplotlib canvas.
    #  */
    def refresh_graph(self, max_points):
        SysLog("Refreshing Matplotlib graph canvas. Max points: %d", max_points)
        global_max_x, global_min_x = 0, 0
        global_max_dist, global_max_motor = 0, 0
        
        # /* Iterate through the registry to update plotting lines */
        for ecu_name, ecu_data in self.ecu_registry.items():
            stats = ecu_data["stats"]
            lines = ecu_data["lines"]
            
            # /* Guard against rendering unpopulated arrays */
            if not stats["dist_sync_history"]: 
                # /* Skip iteration */
                continue

            x_data = list(range(max(0, stats["received"] - len(stats["dist_sync_history"])), stats["received"]))
            
            # /* Conditional block to recalculate viewing boundaries */
            if x_data:
                global_max_x = max(global_max_x, max(x_data))
                global_min_x = max(0, global_max_x - max_points)
                global_max_dist = max(global_max_dist, max(stats["d0_history"]), max(stats["d1_history"]), max(stats["d2_history"]))
                global_max_motor = max(global_max_motor, max(stats["m0_history"]), max(stats["m1_history"]))

            lines["dist_seq"].set_data(x_data, stats["dist_sync_history"])
            lines["d0"].set_data(x_data, stats["d0_history"])
            lines["d1"].set_data(x_data, stats["d1_history"])
            lines["d2"].set_data(x_data, stats["d2_history"])
            lines["m0"].set_data(x_data, stats["m0_history"])
            lines["m1"].set_data(x_data, stats["m1_history"])

        self.ax1.set_xlim(global_min_x, global_max_x + 1)
        self.ax1.set_ylim(-0.5, 3.5)
        
        self.ax2.set_xlim(global_min_x, global_max_x + 1)
        self.ax2.set_ylim(-10, (global_max_dist * 1.2) if global_max_dist > 0 else 1023)
        
        self.ax3.set_xlim(global_min_x, global_max_x + 1)
        self.ax3.set_ylim(-10, (global_max_motor * 1.2) if global_max_motor > 0 else 1023)
        
        self.canvas.draw()
        
        # /* Return from execution */
        return

    # /* ===================================================================== */
    # /* USER ACTION CALLBACKS                                                 */
    # /* ===================================================================== */

    # /*
    #  * @brief Callback invoked when the Start/Stop button is pressed.
    #  * Transmits the System Control toggle command to all registered ECUs.
    #  */
    def on_start_stop_clicked(self):
        self.sys_engine_started = not getattr(self, 'sys_engine_started', False)
        state_str = "START" if self.sys_engine_started else "STOP"
        
        self.app_log(f"ACTION: System {state_str} command initiated.")
        SysInfo("ACTION: System %s command initiated.", state_str)
        payload = CCUCommandBuilder.build_sys_ctrl(self.sys_engine_started)
        
        # /* Loop through registered ECUs to broadcast the system state */
        for zone, ecu in self.zone_ecus.items():
            # /* Branch if target ECU exists in the registry */
            if ecu:
                self.UDPSendBack(ecu, payload)
                
        # /* Return from execution */
        return

    # /*
    #  * @brief Callback invoked when the Move Forward button is pressed.
    #  * Forces 100% Forward Command across all engines.
    #  */
    def on_forward_clicked(self):
        self.app_log("ACTION: Move Forward command initiated.")
        SysInfo("ACTION: Move Forward command initiated.")
        payload = CCUCommandBuilder.build_eng_ctrl(100, 100)
        
        # /* Loop through registered ECUs to broadcast the forward state */
        for zone, ecu in self.zone_ecus.items():
            # /* Branch if target ECU exists in the registry */
            if ecu:
                self.UDPSendBack(ecu, payload)
                
        # /* Return from execution */
        return

    # /*
    #  * @brief Callback invoked when the Move Backward button is pressed.
    #  * Forces 100% Backward Command across all engines.
    #  */
    def on_backward_clicked(self):
        self.app_log("ACTION: Move Backward command initiated.")
        SysInfo("ACTION: Move Backward command initiated.")
        payload = CCUCommandBuilder.build_eng_ctrl(-100, -100)
        
        # /* Loop through registered ECUs to broadcast the backward state */
        for zone, ecu in self.zone_ecus.items():
            # /* Branch if target ECU exists in the registry */
            if ecu:
                self.UDPSendBack(ecu, payload)
                
        # /* Return from execution */
        return

    # /*
    #  * @brief Helper function to dispatch motor updates to a specific zone.
    #  * @param zone String representing "Front" or "Back".
    #  */
    def _transmit_motor_update(self, zone):
        SysLog("Transmitting motor update for zone: %s", zone)
        ecu = self.zone_ecus.get(zone)
        
        # /* Branch if ECU is currently registered and actively connected */
        if ecu:
            val_l = self.motor_states[zone]["L"]
            val_r = self.motor_states[zone]["R"]
            payload = CCUCommandBuilder.build_eng_ctrl(val_l, val_r)
            self.UDPSendBack(ecu, payload)
            
        # /* Return from execution */
        return

    # /*
    #  * @brief Callback invoked when the Front Left slider is adjusted.
    #  * @param value The new integer value of the slider.
    #  */
    def on_slider_front_left_changed(self, value):
        self.app_log(f"CONTROL: Front Left Motor adjusted to {value}%")
        SysInfo("CONTROL: Front Left Motor adjusted to %d%%", value)
        self.motor_states["Front"]["L"] = value
        self._transmit_motor_update("Front")
        
        # /* Return from execution */
        return

    # /*
    #  * @brief Callback invoked when the Front Right slider is adjusted.
    #  * @param value The new integer value of the slider.
    #  */
    def on_slider_front_right_changed(self, value):
        self.app_log(f"CONTROL: Front Right Motor adjusted to {value}%")
        SysInfo("CONTROL: Front Right Motor adjusted to %d%%", value)
        self.motor_states["Front"]["R"] = value
        self._transmit_motor_update("Front")
        
        # /* Return from execution */
        return

    # /*
    #  * @brief Callback invoked when the Back Left slider is adjusted.
    #  * @param value The new integer value of the slider.
    #  */
    def on_slider_back_left_changed(self, value):
        self.app_log(f"CONTROL: Back Left Motor adjusted to {value}%")
        SysInfo("CONTROL: Back Left Motor adjusted to %d%%", value)
        self.motor_states["Back"]["L"] = value
        self._transmit_motor_update("Back")
        
        # /* Return from execution */
        return

    # /*
    #  * @brief Callback invoked when the Back Right slider is adjusted.
    #  * @param value The new integer value of the slider.
    #  */
    def on_slider_back_right_changed(self, value):
        self.app_log(f"CONTROL: Back Right Motor adjusted to {value}%")
        SysInfo("CONTROL: Back Right Motor adjusted to %d%%", value)
        self.motor_states["Back"]["R"] = value
        self._transmit_motor_update("Back")
        
        # /* Return from execution */
        return

    # /*
    #  * @brief Handles cleanup procedures before the application window closes.
    #  */
    def closeEvent(self, event):
        self.app_log("Shutting down core networking services...")
        SysInfo("Shutting down core networking services...")
        self.udp_thread.stop()
        self.udp_thread.wait() 
        event.accept()
        
        # /* Return from execution */
        return

    # /* ===================================================================== */
    # /* RESPONSIVE UI OVERRIDE                                                */
    # /* ===================================================================== */

    # /*
    #  * @brief Intercepts window resize events to dynamically scale the Graph.
    #  * @details Forces the fixed-geometry Graph widget to expand and fill the 
    #  * remaining horizontal and vertical space of the main window.
    #  * @param event The QResizeEvent triggered by the OS.
    #  */
    def resizeEvent(self, event):
        super().resizeEvent(event)
        win_w = self.width()
        win_h = self.height()
        
        SysLog("Window resized: %d x %d", win_w, win_h)
        
        new_graph_width = max(200, win_w - 670 - 20)
        new_graph_height = max(200, win_h - 40 - 40)
        self.ui.Graph.setGeometry(670, 40, new_graph_width, new_graph_height)
        
        # /* Return from execution */
        return

# /* ========================================================================= */
# /* EXECUTION ENTRY POINT                                                     */
# /* ========================================================================= */
if __name__ == "__main__":
    SysInfo("Application execution started.")
    app = QApplication(sys.argv)
    window = MyMonitorApp()
    window.show()
    sys.exit(app.exec())