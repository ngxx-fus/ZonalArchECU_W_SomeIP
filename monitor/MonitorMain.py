## @file MonitorMain.py
## @brief Main entry point for the Zonal Architecture Monitor application.

import sys
import time
import socket
import signal
import struct
from datetime import datetime
from PySide6.QtWidgets import (QApplication, QMainWindow, QVBoxLayout, QTextEdit, 
                               QDialog, QTreeView, QMenu, QTreeWidgetItem)
from PySide6.QtCore import Qt, QTimer, QUrl
from collections import deque

from matplotlib.backends.backend_qtagg import FigureCanvasQTAgg
from matplotlib.figure import Figure

from MonitorConfig import *

# /* Import external components */
from monitor_ui import Ui_MainWindow
from monitor_list_zecu import Ui_Dialog as Ui_ListZECU
from monitor_auth import NodeAuth
from CoreNetwork import ZoneECU
from UdpListener import UdpListenerThread

# /* ========================================================================= */
# /* GLOBAL VARIABLES                                                          */
# /* ========================================================================= */
# /* Global GPS coordinates (Ho Chi Minh City default) */
GLOBAL_LAT = 10.762622
GLOBAL_LON = 106.660172
MAX_GSP_MAP_REFRESH_TIME_SEC = 3

# /* Constants for Custom Keep-Alive Protocol */
KEEPALIVE_MAGIC = 0xAA
KEEPALIVE_FRAME_TYPE = 0x5500AA00

# /* Flag to disable Keep-Alive tracking for testing purposes */
DISABLE_KEEPALIVE_TRACK = True

# /* ========================================================================= */
# /* ZECU DISCOVERY DIALOG                                                     */
# /* ========================================================================= */

class DiscoveryDialog(QDialog):
    def __init__(self, parent=None):
        super().__init__(parent)
        
        # /* Explicitly save a Python reference to the parent App to avoid QWidget wrapper issues */
        self.main_app = parent
        self.ui = Ui_ListZECU()
        self.ui.setupUi(self)
        self.setWindowTitle("Discovered ZECUs (Broadcast Scan)")
        
        self.tree = self.ui.treeWidget
        self.tree.setColumnCount(2)
        self.tree.setHeaderLabels(["Name / Service", "Details"])
        
        self.tree.setContextMenuPolicy(Qt.CustomContextMenu)
        self.tree.customContextMenuRequested.connect(self.on_context_menu)
        
        self.ui.pushButton_RemoteZECU.clicked.connect(self.on_remove_zecu_clicked)
        
        self.discovered_ecus = {}
        
        # /* Create independent socket to capture Broadcast packets */
        self.udp_sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        try:
            self.udp_sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
            if hasattr(socket, 'SO_REUSEPORT'):
                self.udp_sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEPORT, 1)
            self.udp_sock.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)
            self.udp_sock.bind(("0.0.0.0", 30490))
        except Exception as e:
            print(f"Warning: Cannot bind Discovery Socket ({e})")
            
        self.udp_sock.setblocking(False)
        
        # /* Scan packets every 100ms */
        self.timer = QTimer(self)
        self.timer.timeout.connect(self.poll_udp)
        self.timer.start(100)
        
    def poll_udp(self):
        try:
            while True:
                data, addr = self.udp_sock.recvfrom(2048)
                print(f"[Discovery] RX: {len(data)} bytes from {addr[0]}")
                self.parse_broadcast(data, addr[0])
        except BlockingIOError:
            pass # Typical for non-blocking sockets when empty
        except socket.error as e:
            if e.errno != 11: # Ignore EAGAIN/EWOULDBLOCK
                print(f"[Discovery] Socket error: {e}")
            
    def parse_broadcast(self, data, ip):
        # /* Check minimum size (88 bytes Header + 8 bytes BodyHeader) */
        if len(data) < 96: 
            print(f"[-] Dropped: Packet too small ({len(data)} < 96)")
            return
        
        # Unpack ZECUFrame_Header_t (88 bytes)
        try:
            magic0, ver, magic1, magic2, label_bytes, mac_bytes, m3, ip_bytes, port, m4, m5, m6, frame_type = struct.unpack_from("<BBBB64s6sB4sHBBBI", data, 0)
        except Exception as e:
            print(f"[-] Unpack Header Error: {e}")
            return
            
        if magic0 != 0xAA or ver != 0x04 or magic1 != 0xAA or magic2 != 0xAA: 
            print(f"[-] Dropped: Magic/Version mismatch ({magic0:02X} {ver:02X} {magic1:02X} {magic2:02X})")
            return
            
        # /* Ensure only Broadcast packets (0xFFFFFFFF) are processed */
        if frame_type != 0xFFFFFFFF:
            return
        
        label = label_bytes.split(b'\x00')[0].decode('utf-8', 'ignore')
        if ip in self.discovered_ecus: return 
            
        # /* Unpack ZECUFrame_Body_Broadcast_t Header (8 bytes) starting from offset 88 */
        magicDword, svc_count, pad = struct.unpack_from("<IB3s", data, 88)
        
        ecu_item = QTreeWidgetItem(self.tree)
        ecu_item.setText(0, f"ZECU: {label}")
        ecu_item.setText(1, f"IPv4: {ip}:{port} | {svc_count} Services Active")
        ecu_item.setData(0, Qt.UserRole, {"ip": ip, "name": label, "port": port})
        
        # /* Parse Services (each service is 64 bytes) */
        offset = 96
        for _ in range(svc_count):
            if offset + 64 > len(data): break
            svc_id, svc_type, svc_acc, svc_desc_bytes = struct.unpack_from("<HHB59s", data, offset)
            svc_desc = svc_desc_bytes.split(b'\x00')[0].decode('utf-8', 'ignore')
            
            s_item = QTreeWidgetItem(ecu_item)
            s_item.setText(0, f"Service: {svc_desc}")
            s_item.setText(1, f"ID: 0x{svc_id:04X} | Type: 0x{svc_type:04X}")
            offset += 64
            
        self.discovered_ecus[ip] = ecu_item
        self.tree.expandAll()
        self.tree.resizeColumnToContents(0)
        
    def on_context_menu(self, pos):
        item = self.tree.itemAt(pos)
        if not item: return
        
        # /* Only show Context Menu if clicked on the root ECU row (not on Service) */
        if item.parent() is None:
            data = item.data(0, Qt.UserRole)
            if data is not None:
                menu = QMenu(self)
                connect_action = menu.addAction(f"Connect to {data['name']}")
                action = menu.exec(self.tree.viewport().mapToGlobal(pos))
                
                if action == connect_action:
                    if self.main_app:
                        self.main_app.connect_to_ecu(data["ip"], data["name"], data["port"])
                    self.accept()
                    
    def on_remove_zecu_clicked(self):
        items = self.tree.selectedItems()
        if not items: return
        
        item = items[0]
        # /* Check if it's the root ECU node */
        if item.parent() is None:
            data = item.data(0, Qt.UserRole)
            if data is not None:
                ecu_name = data["name"]
                ip = data["ip"]
                # /* Call main app to remove */
                if self.main_app:
                    self.main_app.remove_ecu(ecu_name)
                # /* Remove from UI */
                root = self.tree.invisibleRootItem()
                root.removeChild(item)
                # /* Remove from local discovered_ecus dictionary */
                if ip in self.discovered_ecus:
                    del self.discovered_ecus[ip]
                    
    def cleanup(self):
        if self.timer.isActive():
            self.timer.stop()
        if self.udp_sock.fileno() != -1:
            self.udp_sock.close()
            
    def accept(self):
        self.cleanup()
        super().accept()
        
    def reject(self):
        self.cleanup()
        super().reject()
        
    def closeEvent(self, event):
        self.cleanup()
        super().closeEvent(event)

# /* ========================================================================= */
# /* MAIN GUI APPLICATION                                                      */
# /* ========================================================================= */

# /*
#  * @brief Class representing the Main GUI Application.
#  */
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
        self.authenticator = NodeAuth()
        # /* Initialize hardcoded key matching ZECU (0xDEADBEEF) */
        self.authenticator.set_core_key(0xDEADBEEF) 
        
        self.ecu_registry = {}
        
        # /* ZoneECU Classification Dictionary containing lists */
        self.zone_ecus = {
            "Front": [],
            "Back": []
        }
        
        # /* Local State tracking for Command Generation */
        self.sys_engine_started = False
        self.motor_states = {
            "Front": {"L": 0, "R": 0},
            "Back": {"L": 0, "R": 0}
        }
        
        # /* State tracking for Visualization Engine Selection */
        self.vis_selected = {
            "FrontLeft": False,
            "FrontRight": False,
            "BackLeft": False,
            "BackRight": False
        }
        
        self.global_packet_count = 0
        
        self.keepalive_sync = 0
        
        # /* GPS state tracking */
        self.last_map_lat = None
        self.last_map_lon = None
        self.last_map_refresh = 0
        self.map_initialized = False

        self.setup_graph()
        self.setup_logging_console()
        self.connect_signals()
        
        # /* Set up a fixed 1-second refresh timer for the Matplotlib chart */
        self.graph_timer = QTimer(self)
        self.graph_timer.timeout.connect(lambda: self.refresh_graph(MAX_NUM_POINT))
        self.graph_timer.start(1000)

        # /* Set up a fixed 1-second interval to check for GPS updates */
        self.gps_timer = QTimer(self)
        self.gps_timer.timeout.connect(self.refresh_gps_map)
        self.gps_timer.start(1000)
        
        # /* Set up a 300ms Keep-Alive timer to prevent ZECU timeout */
        if not DISABLE_KEEPALIVE_TRACK:
            self.keep_alive_timer = QTimer(self)
            self.keep_alive_timer.timeout.connect(self.send_keep_alive)
            self.keep_alive_timer.start(300)

        self.udp_thread = UdpListenerThread()
        self.udp_thread.data_received.connect(self.process_incoming_data)
        self.udp_thread.start()

        # /* Set default view to Matplotlib Graph */
        self.ui.stackedWidget.setCurrentIndex(0)

        self.app_log("System Ready. Waiting for ECU Heartbeats...")
        SysInfo("System Ready. Waiting for ECU Heartbeats...")

    # /*
    #  * @brief Upgrades the static label into a dynamic QTextEdit for logging.
    #  */
    def setup_logging_console(self):
        SysLog("Upgrading static label to dynamic QTextEdit for logging.")
        geom = self.ui.Visualization_MonitorApp_Status.geometry()
        parent = self.ui.Visualization_MonitorApp_Status.parentWidget()
        
        self.ui.Visualization_MonitorApp_Status.deleteLater()
        
        self.log_console = QTextEdit(parent)
        self.log_console.setGeometry(geom)
        self.log_console.setReadOnly(True)
        self.log_console.document().setMaximumBlockCount(20) 
        
        # /* Return from execution */
        return

    # /*
    #  * @brief Binds all UI events to their respective callback functions.
    #  * Applies dynamic slider ranges based on global configuration.
    #  */
    def connect_signals(self):
        SysLog("Binding UI events to callback functions.")
        
        # /* Apply strict slider constraints based on configurations */
        min_motor = min(MIN_MOTOR_SPEED_VAL)
        max_motor = max(MAX_MOTOR_SPEED_VAL)
        
        self.ui.Control_Front_Motor_Control_Slider_Left.setRange(min_motor, max_motor)
        self.ui.Control_Front_Motor_Control_Slider_Right.setRange(min_motor, max_motor)
        self.ui.Control_Back_Motor_Control_Slider_Left.setRange(min_motor, max_motor)
        self.ui.Control_Back_Motor_Control_Slider_Right.setRange(min_motor, max_motor)

        # /* Connect click events for Visualization Engine Selection */
        self.ui.Visualization_EngineSelect_FrontLeft.clicked.connect(lambda: self.toggle_vis_engine("FrontLeft", self.ui.Visualization_EngineSelect_FrontLeft))
        self.ui.Visualization_EngineSelect_FrontRight.clicked.connect(lambda: self.toggle_vis_engine("FrontRight", self.ui.Visualization_EngineSelect_FrontRight))
        self.ui.Visualization_EngineSelect_BackLeft.clicked.connect(lambda: self.toggle_vis_engine("BackLeft", self.ui.Visualization_EngineSelect_BackLeft))
        self.ui.Visualization_EngineSelect_BackRight.clicked.connect(lambda: self.toggle_vis_engine("BackRight", self.ui.Visualization_EngineSelect_BackRight))
        self.ui.Visualization_EngineSpeed_Set.clicked.connect(self.on_vis_speed_set)

        # /* Connect click and slide events for Control Group */
        self.ui.Control_StartStop.clicked.connect(self.on_start_stop_clicked)
        self.ui.Control_MoveForward.clicked.connect(self.on_forward_clicked)
        self.ui.Control_MoveBackward.clicked.connect(self.on_backward_clicked)
        self.ui.Control_EngineSpeed.clicked.connect(self.on_control_combo_speed_set)
        
        self.ui.Control_Front_Motor_Control_Slider_Left.sliderReleased.connect(self.on_slider_front_left_released)
        self.ui.Control_Front_Motor_Control_Slider_Right.sliderReleased.connect(self.on_slider_front_right_released)
        self.ui.Control_Back_Motor_Control_Slider_Left.sliderReleased.connect(self.on_slider_back_left_released)
        self.ui.Control_Back_Motor_Control_Slider_Right.sliderReleased.connect(self.on_slider_back_right_released)
        
        # /* Verify and connect clear status button if it exists */
        if hasattr(self.ui, 'Visualization_MonitorApp_Status_Clear'):
            self.ui.Visualization_MonitorApp_Status_Clear.clicked.connect(self.on_clear_status_clicked)
            
        # /* Verify and connect copy status button if it exists */
        if hasattr(self.ui, 'Visualization_MonitorApp_Status_Copy'):
            self.ui.Visualization_MonitorApp_Status_Copy.clicked.connect(self.on_copy_status_clicked)

        # /* Connect menu actions */
        self.ui.ConnectZECU.triggered.connect(self.open_discovery_dialog)
        self.ui.actionGPS_map.triggered.connect(self.show_gps_view)
        self.ui.actionECU_status_graph.triggered.connect(self.show_graph_view)
        self.ui.actionZECU_List.triggered.connect(self.open_discovery_dialog)
        
        # /* Return from execution */
        return

    # /*
    #  * @brief Opens the ZECU Discovery UI
    #  */
    def open_discovery_dialog(self):
        self.app_log("Opening ZECU Discovery Dialog...")
        dialog = DiscoveryDialog(self)
        dialog.exec()
        
    # /*
    #  * @brief Retrieves the currently selected ECU name from the UI list.
    #  * @return String name of the ECU, or None if not found/selected.
    #  */
    def get_active_ecu_name(self):
        if hasattr(self.ui, 'ZECU_List'):
            if hasattr(self.ui.ZECU_List, 'currentItem') and self.ui.ZECU_List.currentItem():
                return self.ui.ZECU_List.currentItem().text()
            elif hasattr(self.ui.ZECU_List, 'currentText'):
                return self.ui.ZECU_List.currentText()
        return None

    # /*
    #  * @brief Adds an ECU name to the UI selection list dynamically.
    #  */
    def add_ecu_to_ui_list(self, ecu_name):
        if hasattr(self.ui, 'ZECU_List'):
            if hasattr(self.ui.ZECU_List, 'addItem'):
                if hasattr(self.ui.ZECU_List, 'findItems'):
                    if not self.ui.ZECU_List.findItems(ecu_name, Qt.MatchExactly):
                        self.ui.ZECU_List.addItem(ecu_name)
                elif hasattr(self.ui.ZECU_List, 'findText'):
                    if self.ui.ZECU_List.findText(ecu_name) == -1:
                        self.ui.ZECU_List.addItem(ecu_name)

    # /*
    #  * @brief Removes an ECU name from the UI selection list.
    #  */
    def remove_ecu_from_ui_list(self, ecu_name):
        if hasattr(self.ui, 'ZECU_List'):
            if hasattr(self.ui.ZECU_List, 'findItems'):
                items = self.ui.ZECU_List.findItems(ecu_name, Qt.MatchExactly)
                for item in items:
                    self.ui.ZECU_List.takeItem(self.ui.ZECU_List.row(item))
            elif hasattr(self.ui.ZECU_List, 'findText'):
                idx = self.ui.ZECU_List.findText(ecu_name)
                if idx >= 0:
                    self.ui.ZECU_List.removeItem(idx)

    # /*
    #  * @brief Purges an ECU from registries and stops Keep-Alive transmissions.
    #  */
    def remove_ecu(self, ecu_name):
        self.app_log(f"REMOVING ZECU: {ecu_name}")
        SysInfo("REMOVING ZECU: %s", ecu_name)
        
        # /* Remove from zone_ecus to halt Keep-Alive and command routing */
        for zone in self.zone_ecus:
            target_list = self.zone_ecus[zone]
            for ecu in target_list[:]:
                if ecu.name == ecu_name:
                    target_list.remove(ecu)
                    
        # /* Purge plotting data and lines from Matplotlib */
        if ecu_name in self.ecu_registry:
            lines = self.ecu_registry[ecu_name]["lines"]
            for line_key, line_obj in lines.items():
                try:
                    line_obj.remove()
                except Exception:
                    pass
            del self.ecu_registry[ecu_name]
            
        # /* Refresh legends safely */
        try:
            self.ax1.legend(loc='upper left', fontsize='x-small')
            self.ax1_twin.legend(loc='upper right', fontsize='x-small')
            self.ax2.legend(loc='upper left', fontsize='x-small')
            self.ax3.legend(loc='upper left', fontsize='x-small')
            self.canvas.draw()
        except Exception:
            pass
        
    # /*
    #  * @brief Initiates logic to authenticate and connect to an ECU
    #  */
    def connect_to_ecu(self, ecu_ip, ecu_name, port):
        self.app_log(f"AUTH: Initiating connection to {ecu_name} at {ecu_ip}:{port}...")
        SysInfo("Initiating connection to %s [%s:%d]", ecu_name, ecu_ip, port)
        
        # eFrameType_PairingRequest = 0x55000001, ePairingRequest_New = 0x0FFCC01E
        body = struct.pack("<BIBBB", 0xAA, 0x0FFCC01E, 0xAA, 0xAA, 0xAA)
        frame = self.build_zecu_frame(0x55000001, body)
        
        self.udp_thread.sock.sendto(frame, (ecu_ip, port))
        self.app_log(f"AUTH: Sent PairingRequest to {ecu_name}. Waiting for AuthTX...")

    # /*
    #  * @brief Handles the incoming AuthTX Challenge from a ZECU
    #  */
    def handle_auth_tx(self, payload):
        challenge = payload["challenge"]
        ecu_ip = payload["ip"]
        ecu_port = payload["port"]
        ecu_name = payload["name"]
        
        self.app_log(f"AUTH: Received Challenge (0x{challenge:016X}) from {ecu_name}.")
        
        try:
            signature = self.authenticator.sign_challenge(challenge)
            self.app_log(f"AUTH: Generated Signature (0x{signature:016X}). Sending AuthRX...")
            
            # eFrameType_AuthRX = 0xAA00000B
            body = struct.pack("<BQBBB", 0xAA, signature, 0xAA, 0xAA, 0xAA)
            frame = self.build_zecu_frame(0xAA00000B, body)
            
            self.udp_thread.sock.sendto(frame, (ecu_ip, ecu_port))
            self.app_log(f"AUTH: Successfully responded AuthRX to {ecu_name}.")
        except Exception as e:
            self.app_log(f"AUTH ERROR: {e}")
            SysErr("AUTH ERROR: %s", str(e))
            
    def calc_checksum16(self, data):
        csum = sum(data)
        while csum >> 16:
            csum = (csum & 0xFFFF) + (csum >> 16)
        return (~csum) & 0xFFFF

    def calc_crc32(self, data):
        crc = 0xFFFFFFFF
        for b in data:
            crc ^= b
            for _ in range(8):
                if crc & 1:
                    crc = (crc >> 1) ^ 0xEDB88320
                else:
                    crc >>= 1
        return crc
        
    def build_zecu_frame(self, frame_type, body_data):
        # Pad body strictly to 520 bytes (ZECUFrame_Body_t Max Size)
        body = body_data.ljust(520, b'\x00')
        
        # Header (88 bytes strictly aligned)
        header = struct.pack("<BBBB64s6sB4sHBBBI",
            0xAA, 0x04, 0xAA, 0xAA,
            b"CentralControlUnit",
            bytes.fromhex("e466e5984fd7"),
            0xAA, socket.inet_aton("10.0.0.102"), 30490,
            0xAA, 0xAA, 0xAA,
            frame_type
        )
        
        payload = header + body # 608 bytes payload
        csum = self.calc_checksum16(payload)
        crc = self.calc_crc32(payload) ^ 0xFFFFFFFF
        
        trailer = struct.pack("<BHBI", 0xAA, csum, 0xAA, crc)
        return payload + trailer
        
    # /*
    #  * @brief Verifies active internet connection via DNS port probe.
    #  * @return True if connected, False otherwise.
    #  */
    def check_internet_connection(self):
        try:
            socket.create_connection(("8.8.8.8", 53), timeout=2)
            # /* Return true on successful connection */
            return True
        except OSError:
            # /* Return false on connection failure */
            return False

    # /*
    #  * @brief Periodically checks and updates the GPS Map view using Leaflet.js.
    #  * Ensures zoom state is preserved by only injecting JS on coordinate change.
    #  */
    def refresh_gps_map(self):
        # /* Evaluate if the GPS view is currently active */
        if self.ui.stackedWidget.currentIndex() == 1:
            # /* Check if internet connection is available */
            if not self.check_internet_connection():
                self.app_log("ERROR: No internet connection for GPS Map.")
                SysErr("No internet connection for GPS Map.")
                error_html = "<html><body style='background-color:#2b2b2b; color:white; font-family:sans-serif;'><h2 style='text-align:center; margin-top:20%;'>No Internet Connection</h2><p style='text-align:center;'>Please check your network settings.</p></body></html>"
                self.ui.Graph_ECUStatus_GPSMap.setHtml(error_html)
                # /* Terminate execution due to network failure */
                return
                
            current_time = time.time()
            
            # /* Initialize the Map layer on first run to maintain dynamic zoom and panning */
            if not self.map_initialized:
                leaflet_html = f"""
                <!DOCTYPE html>
                <html>
                <head>
                    <link rel="stylesheet" href="https://unpkg.com/leaflet@1.9.4/dist/leaflet.css" />
                    <script src="https://unpkg.com/leaflet@1.9.4/dist/leaflet.js"></script>
                    <style> body {{ margin:0; padding:0; }} #map {{ height: 100vh; width: 100vw; }} </style>
                </head>
                <body>
                    <div id="map"></div>
                    <script>
                        var map = L.map('map').setView([{GLOBAL_LAT}, {GLOBAL_LON}], 15);
                        L.tileLayer('https://{{s}}.tile.openstreetmap.org/{{z}}/{{x}}/{{y}}.png', {{
                            attribution: '&copy; OpenStreetMap contributors'
                        }}).addTo(map);
                        var marker = L.marker([{GLOBAL_LAT}, {GLOBAL_LON}]).addTo(map);
                        
                        function updateLocation(lat, lon) {{
                            var newLatLng = new L.LatLng(lat, lon);
                            marker.setLatLng(newLatLng);
                            map.panTo(newLatLng);
                        }}
                    </script>
                </body>
                </html>
                """
                self.ui.Graph_ECUStatus_GPSMap.setHtml(leaflet_html)
                self.map_initialized = True
                self.last_map_lat = GLOBAL_LAT
                self.last_map_lon = GLOBAL_LON
                self.last_map_refresh = current_time
                # /* Terminate execution after initialization */
                return

            # /* Check if coordinates have changed to avoid unnecessary updates */
            if self.last_map_lat == GLOBAL_LAT and self.last_map_lon == GLOBAL_LON:
                # /* Terminate execution if position is unchanged */
                return
                
            # /* Throttle refresh rate based on global configuration */
            if (current_time - self.last_map_refresh) < MAX_GSP_MAP_REFRESH_TIME_SEC:
                # /* Terminate execution to enforce rate limiting */
                return
                
            self.last_map_lat = GLOBAL_LAT
            self.last_map_lon = GLOBAL_LON
            self.last_map_refresh = current_time
            
            # /* Execute JavaScript dynamically to update marker and pan map without reloading */
            js_code = f"updateLocation({GLOBAL_LAT}, {GLOBAL_LON});"
            self.ui.Graph_ECUStatus_GPSMap.page().runJavaScript(js_code)
                
        # /* Return from execution */
        return

    # /*
    #  * @brief Switches the UI stack to the Graph View.
    #  */
    def show_graph_view(self):
        self.ui.stackedWidget.setCurrentIndex(0)
        self.app_log("UI: Switched to Matplotlib Graph View.")
        
        # /* Return from execution */
        return

    # /*
    #  * @brief Switches the UI stack to the GPS Map View.
    #  */
    def show_gps_view(self):
        self.ui.stackedWidget.setCurrentIndex(1)
        self.refresh_gps_map()
        self.app_log("UI: Switched to GPS Map View.")
        
        # /* Return from execution */
        return

    # /*
    #  * @brief Toggles the active page of the main QStackedWidget.
    #  */
    def toggle_view(self):
        current_idx = self.ui.stackedWidget.currentIndex()
        
        # /* Determine the next index to switch to */
        if current_idx == 0:
            self.show_gps_view()
        else:
            self.show_graph_view()
            
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
    #  * @brief Clears the status log console.
    #  */
    def on_clear_status_clicked(self):
        SysLog("Clearing status log console.")
        self.log_console.clear()
        
        # /* Return from execution */
        return

    # /*
    #  * @brief Copies the current text in the status log to the system clipboard.
    #  */
    def on_copy_status_clicked(self):
        SysLog("Copying status log to clipboard.")
        clipboard = QApplication.clipboard()
        clipboard.setText(self.log_console.toPlainText())
        
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
            # /* Return early to prevent transmission failure */
            return
            
        try:
            self.udp_thread.sock.sendto(data, (target_ecu.ip, target_ecu.port))
            self.app_log(f"TX -> {target_ecu.name} [{target_ecu.ip}:{target_ecu.port}] | Size: {len(data)} bytes")
            SysInfo("TX -> %s [%s:%d] | Size: %d bytes", target_ecu.name, target_ecu.ip, target_ecu.port, len(data))
        except Exception as e:
            self.app_log(f"TX ERROR: Failed to transmit to {target_ecu.name}. Reason: {e}")
            SysErr("TX ERROR: Failed to transmit to %s. Reason: %s", target_ecu.name, str(e))
            
        # /* Return from execution */
        return

    # /*
    #  * @brief Transmits a dedicated Keep-Alive frame every 300ms to maintain connection.
    #  * Evaluates a rolling sync counter (0 to 3) with the predefined Magic Number.
    #  */
    def send_keep_alive(self):
        if DISABLE_KEEPALIVE_TRACK:
            return
            
        body = struct.pack("<BBBB", KEEPALIVE_MAGIC, self.keepalive_sync, KEEPALIVE_MAGIC, KEEPALIVE_MAGIC)
        payload = self.build_zecu_frame(KEEPALIVE_FRAME_TYPE, body)
        
        self.keepalive_sync = (self.keepalive_sync + 1) % 4
        
        for zone, ecu_list in self.zone_ecus.items():
            if not ecu_list:
                continue
                
            for ecu in ecu_list:
                if ecu is None or not ecu.ip:
                    continue
                try:
                    self.udp_thread.sock.sendto(payload, (ecu.ip, ecu.port))
                except Exception:
                    pass
        return

    # /*
    #  * @brief Initializes the embedded Matplotlib charting environment.
    #  * @details Also configures layouts for the QStackedWidget pages to ensure proper scaling.
    #  */
    def setup_graph(self):
        SysLog("Initializing embedded Matplotlib charting environment.")
        self.fig = Figure(figsize=(5, 4), dpi=100)
        self.fig.tight_layout()
        
        self.ax1 = self.fig.add_subplot(311)
        self.ax1.set_ylabel('Sync (0-3)')
        self.ax1.grid(True)
        
        self.ax1_twin = self.ax1.twinx()
        self.ax1_twin.set_ylabel('Cycle (ms)')
        
        self.ax2 = self.fig.add_subplot(312, sharex=self.ax1)
        self.ax2.set_ylabel('Dist (cm)')
        self.ax2.grid(True)
        
        self.ax3 = self.fig.add_subplot(313, sharex=self.ax1)
        self.ax3.set_xlabel('Packet Index')
        self.ax3.set_ylabel('Motor (RPM)')
        self.ax3.grid(True)
        
        self.canvas = FigureCanvasQTAgg(self.fig)
        
        # /* Configure layout for the first page of the stacked widget to automatically expand its children */
        stack_layout = QVBoxLayout(self.ui.Graph_Stack)
        stack_layout.setContentsMargins(0, 0, 0, 0)
        stack_layout.addWidget(self.ui.Graph_ECUStatus)
        
        layout = QVBoxLayout(self.ui.Graph_ECUStatus)
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
            "dist_sync_history": deque(maxlen=MAX_NUM_POINT), "motor_sync_history": deque(maxlen=MAX_NUM_POINT),
            "d0_history": deque(maxlen=MAX_NUM_POINT), "d1_history": deque(maxlen=MAX_NUM_POINT), "d2_history": deque(maxlen=MAX_NUM_POINT),
            "m0_history": deque(maxlen=MAX_NUM_POINT), "m1_history": deque(maxlen=MAX_NUM_POINT), "avg_cycle_history": deque(maxlen=MAX_NUM_POINT)
        }
        
        lines = {
            "dist_seq": self.ax1.plot([], [], marker='o', markersize=3, label=f'[{ecu_name}] Dist Sync')[0],
            "motor_seq": self.ax1.plot([], [], marker='x', markersize=3, label=f'[{ecu_name}] Motor Sync')[0],
            "avg_cycle": self.ax1_twin.plot([], [], linestyle='--', color='red', markersize=2, alpha=0.7, label=f'[{ecu_name}] Cycle (ms)')[0],
            "d0": self.ax2.plot([], [], marker='o', markersize=3, label=f'[{ecu_name}] D0')[0],
            "d1": self.ax2.plot([], [], marker='x', markersize=3, label=f'[{ecu_name}] D1')[0],
            "d2": self.ax2.plot([], [], marker='^', markersize=3, label=f'[{ecu_name}] D2')[0],
            "m0": self.ax3.plot([], [], marker='o', markersize=3, label=f'[{ecu_name}] M0')[0],
            "m1": self.ax3.plot([], [], marker='x', markersize=3, label=f'[{ecu_name}] M1')[0],
        }
        
        self.ax1.legend(loc='upper left', fontsize='x-small')
        self.ax1_twin.legend(loc='upper right', fontsize='x-small')
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
        msg_type = payload.get("type", "HeartBeat")
        if msg_type == "AuthTX":
            self.handle_auth_tx(payload)
            return
            
        self.global_packet_count += 1
        ecu_name = payload["name"]
        
        SysLog("Processing incoming packet from ECU: %s", ecu_name)
        
        ecu_type = None
        ecu_name_lower = ecu_name.lower()
        # /* Check if ECU is classified as Front */
        if "front" in ecu_name_lower:
            ecu_type = "Front"
        # /* Check if ECU is classified as Back */
        elif "back" in ecu_name_lower or "rear" in ecu_name_lower:
            ecu_type = "Back"
            
        # /* Conditional steering to validate ECU assignment */
        if ecu_type:
            ecu_list = self.zone_ecus[ecu_type]
            
            # /* Search for existing ECU in the list */
            existing_ecu = next((ecu for ecu in ecu_list if ecu.name == ecu_name), None)
            
            # /* Branch if ECU is not currently registered */
            if existing_ecu is None:
                max_allowed = MAX_NUM_FRONT_ZECU if ecu_type == "Front" else MAX_NUM_BACK_ZECU
                
                # /* Check if the current zone has reached its maximum capacity */
                if len(ecu_list) >= max_allowed:
                    # /* Branch if auto-replace is enabled */
                    if ALLOW_AUTO_REPACE_ZECU == 1:
                        oldest_ecu = ecu_list[0]
                        self.app_log(f"DROPPED: {ecu_type} Zone -> {oldest_ecu.name} (Auto-replace)")
                        SysInfo("DROPPED: %s Zone -> %s", ecu_type, oldest_ecu.name)
                        
                        # /* Safely purge old ECU before replacing */
                        self.remove_ecu(oldest_ecu.name)
                        self.remove_ecu_from_ui_list(oldest_ecu.name)
                        
                        self.zone_ecus[ecu_type].append(ZoneECU(ecu_name, payload["ip"], payload["port"], payload["mac"]))
                        self.app_log(f"REGISTERED: {ecu_type} Zone -> {ecu_name}")
                        self.add_ecu_to_ui_list(ecu_name)
                    else:
                        self.app_log(f"ERROR: Capacity full in {ecu_type} Zone. Dropping {ecu_name}")
                        SysErr("Capacity full in %s Zone. Auto-replace disabled.", ecu_type)
                        # /* Return from execution to discard packet */
                        return
                else:
                    ecu_list.append(ZoneECU(ecu_name, payload["ip"], payload["port"], payload["mac"]))
                    self.app_log(f"REGISTERED: {ecu_type} Zone -> {ecu_name}")
                    self.add_ecu_to_ui_list(ecu_name)
            else:
                # /* Check if the IP or port has changed to update */
                if existing_ecu.ip != payload["ip"] or existing_ecu.port != payload["port"]:
                    existing_ecu.ip = payload["ip"]
                    existing_ecu.port = payload["port"]
                    self.app_log(f"UPDATED: {ecu_type} Zone -> {ecu_name} connection details")
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
        
        # /* Cycle time calculation based on Sync packet timestamps */
        current_time = payload["timestamp"]
        # /* Check if there is a previous timestamp to calculate difference */
        if stats["last_time"] != 0.0:
            cycle_ms = (current_time - stats["last_time"]) * 1000.0
        else:
            cycle_ms = 0.0
        stats["last_time"] = current_time
        
        stats["dist_sync_history"].append(payload["dist_sync"])
        stats["motor_sync_history"].append(payload["motor_sync"])
        stats["avg_cycle_history"].append(cycle_ms)
        stats["d0_history"].append(payload["d0"])
        stats["d1_history"].append(payload["d1"])
        stats["d2_history"].append(payload["d2"])
        stats["m0_history"].append(payload["m0"])
        stats["m1_history"].append(payload["m1"])
        
        self.update_gui_elements(ecu_name, payload["d0"], payload["d1"], payload["d2"], payload["m0"], payload["m1"])

        # /* Return from execution */
        return

    # /*
    #  * @brief Updates numeric labels, internal states, and sliders based on telemetry.
    #  */
    def update_gui_elements(self, ecu_name, d0, d1, d2, m0, m1):
        active_ecu = self.get_active_ecu_name()
        
        # /* If a specific ECU is selected, ignore updates from others to prevent UI flickering */
        if active_ecu and ecu_name != active_ecu:
            return

        SysLog("Updating GUI elements for ECU: %s", ecu_name)
        f_d0 = f"{d0/100:.2f}m"
        f_d1 = f"{d1/100:.2f}m"
        f_d2 = f"{d2/100:.2f}m"
        
        ecu_name_lower = ecu_name.lower()
        # /* Evaluate ECU origin to route data to the appropriate UI cluster */
        if "front" in ecu_name_lower:
            self.motor_states["Front"]["L"] = m0
            self.motor_states["Front"]["R"] = m1
            
            self.ui.Visualization_Front_Distance_Sensor_Left.setText(f_d0)
            self.ui.Visualization_Front_Distance_Sensor_Center.setText(f_d1)
            self.ui.Visualization_Front_Distance_Sensor_Right.setText(f_d2)
            
            self.ui.Control_Front_Motor_Control_Slider_Left.blockSignals(True)
            self.ui.Control_Front_Motor_Control_Slider_Right.blockSignals(True)
            
            self.ui.Control_Front_Motor_Control_Slider_Left.setValue(m0)
            self.ui.Control_Front_Motor_Control_Slider_Right.setValue(m1)
            
            self.ui.Control_Front_Motor_Control_Slider_Left.blockSignals(False)
            self.ui.Control_Front_Motor_Control_Slider_Right.blockSignals(False)
            
        elif "back" in ecu_name_lower or "rear" in ecu_name_lower:
            self.motor_states["Back"]["L"] = m0
            self.motor_states["Back"]["R"] = m1
            
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
    #  * @param max_points The maximum number of historical points evaluated.
    #  */
    def refresh_graph(self, max_points):
        SysLog("Refreshing Matplotlib graph canvas. Max points: %d", max_points)
        global_max_x, global_min_x = 0, 0
        global_max_dist, global_max_motor = 0, 0
        global_max_cycle = 0
        
        # /* Iterate through the registry to update plotting lines */
        for ecu_name, ecu_data in self.ecu_registry.items():
            stats = ecu_data["stats"]
            lines = ecu_data["lines"]
            
            # /* Guard against rendering unpopulated arrays */
            if not stats["dist_sync_history"]: 
                # /* Continue to the next loop iteration */
                continue

            x_data = list(range(max(0, stats["received"] - len(stats["dist_sync_history"])), stats["received"]))
            
            # /* Conditional block to recalculate viewing boundaries */
            if x_data:
                global_max_x = max(global_max_x, max(x_data))
                global_min_x = max(0, global_max_x - max_points)
                global_max_dist = max(global_max_dist, max(stats["d0_history"]), max(stats["d1_history"]), max(stats["d2_history"]))
                global_max_motor = max(global_max_motor, max(map(abs, stats["m0_history"])), max(map(abs, stats["m1_history"])))
                global_max_cycle = max(global_max_cycle, max(stats["avg_cycle_history"]))

            lines["dist_seq"].set_data(x_data, stats["dist_sync_history"])
            
            # /* Apply a slight vertical offset to Motor Sync so it doesnt completely overlap Dist Sync */
            motor_sync_offset = [v + 0.1 for v in stats["motor_sync_history"]]
            lines["motor_seq"].set_data(x_data, motor_sync_offset)
            lines["avg_cycle"].set_data(x_data, stats["avg_cycle_history"])
            lines["d0"].set_data(x_data, stats["d0_history"])
            lines["d1"].set_data(x_data, stats["d1_history"])
            lines["d2"].set_data(x_data, stats["d2_history"])
            lines["m0"].set_data(x_data, stats["m0_history"])
            lines["m1"].set_data(x_data, stats["m1_history"])

        self.ax1.set_xlim(global_min_x, global_max_x + 1)
        self.ax1.set_ylim(-0.5, 3.5)
        
        self.ax1_twin.set_xlim(global_min_x, global_max_x + 1)
        self.ax1_twin.set_ylim(0, max(10, global_max_cycle * 1.2))
        
        self.ax2.set_xlim(global_min_x, global_max_x + 1)
        self.ax3.set_xlim(global_min_x, global_max_x + 1)

        # /* Check global configuration for dynamic vs static scaling */
        if SCALE_BASE_ON_LOCAL_MAX_VAL == 1:
            self.ax2.set_ylim(-10, (global_max_dist * 1.2) if global_max_dist > 0 else 1023)
            
            max_m = max(10, global_max_motor)
            self.ax3.set_ylim(-max_m * 1.2, max_m * 1.2)
        else:
            # /* Enforce limits from configuration file */
            max_dist_cm = max(MAX_DISTANCE_VAL) * 100 
            self.ax2.set_ylim(-10, max_dist_cm)
            self.ax3.set_ylim(min(MIN_MOTOR_SPEED_VAL), max(MAX_MOTOR_SPEED_VAL))
        
        self.fig.tight_layout()
        self.canvas.draw()
        
        # /* Return from execution */
        return

    # /* ===================================================================== */
    # /* USER ACTION CALLBACKS                                                 */
    # /* ===================================================================== */

    # /*
    #  * @brief Toggles the selection state and visual style of a Visualization engine button.
    #  * @param engine_id String key representing the engine.
    #  * @param btn_widget The UI button widget reference.
    #  */
    def toggle_vis_engine(self, engine_id, btn_widget):
        self.vis_selected[engine_id] = not self.vis_selected[engine_id]
        
        # /* Apply styling based on selection state */
        if self.vis_selected[engine_id]:
            btn_widget.setStyleSheet("background-color: green; color: white;")
        else:
            btn_widget.setStyleSheet("")
            
        # /* Return from execution */
        return

    # /*
    #  * @brief Processes the Set request from the Visualization group.
    #  * Transmits motor update commands for any selected engines.
    #  */
    def on_vis_speed_set(self):
        val = self.ui.Visualization_EngineSpeedSpinBox.value()
        
        update_fl = self.vis_selected["FrontLeft"]
        update_fr = self.vis_selected["FrontRight"]
        update_bl = self.vis_selected["BackLeft"]
        update_br = self.vis_selected["BackRight"]

        # /* Transmit data if any front engine was modified */
        if update_fl or update_fr:
            if update_fl: self.motor_states["Front"]["L"] = val
            if update_fr: self.motor_states["Front"]["R"] = val
            self._transmit_motor_update("Front", update_l=update_fl, update_r=update_fr)

        # /* Transmit data if any back engine was modified */
        if update_bl or update_br:
            if update_bl: self.motor_states["Back"]["L"] = val
            if update_br: self.motor_states["Back"]["R"] = val
            self._transmit_motor_update("Back", update_l=update_bl, update_r=update_br)

        # /* Reset selection to prevent stuck buttons on next trigger */
        self.vis_selected = { k: False for k in self.vis_selected }
        self.ui.Visualization_EngineSelect_FrontLeft.setStyleSheet("")
        self.ui.Visualization_EngineSelect_FrontRight.setStyleSheet("")
        self.ui.Visualization_EngineSelect_BackLeft.setStyleSheet("")
        self.ui.Visualization_EngineSelect_BackRight.setStyleSheet("")

        self.app_log(f"VISUALIZATION: Speed updated to {val} for selected engines.")
        
        # /* Return from execution */
        return

    # /*
    #  * @brief Processes the Set request from the Control group using comboboxes.
    #  */
    def on_control_combo_speed_set(self):
        val = self.ui.Control_EngineSpeedSpinBox.value()
        front_sel = self.ui.Control_EngineSelect_Front_2.currentText()
        back_sel = self.ui.Control_EngineSelect_Front.currentText()

        # /* Identify whether the user is modifying Front or Back based on active text */
        if "Front" in front_sel:
            update_fl, update_fr = False, False
            # /* Evaluate front combobox selection routing */
            if front_sel == "Front_LR":
                self.motor_states["Front"]["L"] = val
                self.motor_states["Front"]["R"] = val
                update_fl, update_fr = True, True
            elif front_sel == "Front_L":
                self.motor_states["Front"]["L"] = val
                update_fl = True
            elif front_sel == "Front_R":
                self.motor_states["Front"]["R"] = val
                update_fr = True

            # /* Transmit data if any front parameters were modified */
            if update_fl or update_fr:
                self._transmit_motor_update("Front", update_l=update_fl, update_r=update_fr)
                self.app_log(f"CONTROL: Combobox Front Speed updated to {val}.")

        if "Back" in back_sel:
            update_bl, update_br = False, False
            # /* Evaluate back combobox selection routing */
            if back_sel == "Back_LR":
                self.motor_states["Back"]["L"] = val
                self.motor_states["Back"]["R"] = val
                update_bl, update_br = True, True
            elif back_sel == "Back_L":
                self.motor_states["Back"]["L"] = val
                update_bl = True
            elif back_sel == "Back_R":
                self.motor_states["Back"]["R"] = val
                update_br = True

            # /* Transmit data if any back parameters were modified */
            if update_bl or update_br:
                self._transmit_motor_update("Back", update_l=update_bl, update_r=update_br)
                self.app_log(f"CONTROL: Combobox Back Speed updated to {val}.")
        
        # /* Return from execution */
        return

    # /*
    #  * @brief Callback invoked when the Start/Stop button is pressed.
    #  * Transmits the System Control toggle command to all registered ECUs.
    #  */
    def on_start_stop_clicked(self):
        self.sys_engine_started = not getattr(self, 'sys_engine_started', False)
        state_str = "START" if self.sys_engine_started else "STOP"
        
        self.app_log(f"ACTION: System {state_str} command initiated.")
        SysInfo("ACTION: System %s command initiated.", state_str)
        
        # /* Route the System Control to valid ZECUFrame payload types */
        if not self.sys_engine_started:
            # /* STOP -> Send EmergencyStop frame */
            body = struct.pack("<BIBHHHBbbBIIBBH", 0xAA, 0xFFFFFFFF, 0xAA, 0, 0, 0, 0xAA, 0, 0, 0xAA, 0, 0, 0xAA, 0xAA, 0)
            payload = self.build_zecu_frame(0xF0F0F0FA, body) # eFrameType_EmergencyStop
        else:
            # /* START -> Send EngineControl3 with zero speed */
            body = struct.pack("<BBBbBbH", 0xAA, 0, 0xAA, 0, 0xAA, 0, 0)
            payload = self.build_zecu_frame(0xFF00CC0C, body) # eFrameType_EngineControl3
        
        active_ecu = self.get_active_ecu_name()
        
        # /* Loop through registered ECU zones */
        for zone, ecu_list in self.zone_ecus.items():
            # /* Iterate through each ECU in the zone */
            for ecu in ecu_list:
                if active_ecu and ecu.name != active_ecu:
                    continue
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
        body = struct.pack("<BBBbBbH", 0xAA, 0, 0xAA, 100, 0xAA, 100, 0)
        payload = self.build_zecu_frame(0xFF00CC0C, body)
        
        active_ecu = self.get_active_ecu_name()
        
        # /* Loop through registered ECUs to broadcast the forward state */
        for zone, ecu_list in self.zone_ecus.items():
            # /* Transmit to every unit in current zone */
            for ecu in ecu_list:
                if active_ecu and ecu.name != active_ecu:
                    continue
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
        body = struct.pack("<BBBbBbH", 0xAA, 0, 0xAA, -100, 0xAA, -100, 0)
        payload = self.build_zecu_frame(0xFF00CC0C, body)
        
        active_ecu = self.get_active_ecu_name()
        
        # /* Loop through registered ECUs to broadcast the backward state */
        for zone, ecu_list in self.zone_ecus.items():
            # /* Transmit to every unit in current zone */
            for ecu in ecu_list:
                if active_ecu and ecu.name != active_ecu:
                    continue
                self.UDPSendBack(ecu, payload)
                
        # /* Return from execution */
        return

    # /*
    #  * @brief Helper function to dispatch motor updates to a specific zone.
    #  * @param zone String representing "Front" or "Back".
    #  * @param update_l Boolean flag to update Left motor.
    #  * @param update_r Boolean flag to update Right motor.
    #  */
    def _transmit_motor_update(self, zone, update_l=True, update_r=True):
        SysLog("Transmitting motor update for zone: %s (L:%s, R:%s)", zone, update_l, update_r)
        ecu_list = self.zone_ecus.get(zone, [])
        
        val_l = self.motor_states[zone]["L"]
        val_r = self.motor_states[zone]["R"]
        
        # /* Control flow: determine FrameType and dynamically pack body based on target motors */
        if update_l and update_r:
            frame_type = 0xFF00CC0C  # /* eFrameType_EngineControl3 (Both M0 & M1) */
            # /* Layout: Magic0(B), Sync(B), Magic1(B), M0(b), Magic2(B), M1(b), CheckSum(H) */
            body = struct.pack("<BBBbBbH", 0xAA, 0, 0xAA, val_l, 0xAA, val_r, 0)
            
        elif update_l:
            frame_type = 0xFF00CC0A  # /* eFrameType_EngineControl1 (M0 only) */
            # /* Layout: Magic0(B), Sync(B), Magic1(B), M0(b), Magic2(B), Magic3(B), CheckSum(H) */
            body = struct.pack("<BBBbBBH", 0xAA, 0, 0xAA, val_l, 0xAA, 0xAA, 0)
            
        elif update_r:
            frame_type = 0xFF00CC0B  # /* eFrameType_EngineControl2 (M1 only) */
            # /* Layout: Magic0(B), Sync(B), Magic1(B), M1(b), Magic2(B), Magic3(B), CheckSum(H) */
            body = struct.pack("<BBBbBBH", 0xAA, 0, 0xAA, val_r, 0xAA, 0xAA, 0)
            
        else:
            # /* Control flow: return early if no motors to update */
            return
            
        payload = self.build_zecu_frame(frame_type, body)
        
        active_ecu = self.get_active_ecu_name()
        
        # /* Control flow: Loop through targeted ECU list */
        for ecu in ecu_list:
            if active_ecu and ecu.name != active_ecu:
                continue
            self.UDPSendBack(ecu, payload)
            
        # /* Return from execution */
        return

    # /*
    #  * @brief Callback invoked when the Front Left slider is adjusted.
    #  */
    def on_slider_front_left_released(self):
        value = self.ui.Control_Front_Motor_Control_Slider_Left.value()
        self.app_log(f"CONTROL: Front Left Motor adjusted to {value}%")
        SysInfo("CONTROL: Front Left Motor adjusted to %d%%", value)
        self.motor_states["Front"]["L"] = value
        self._transmit_motor_update("Front", update_l=True, update_r=False)
        
        # /* Return from execution */
        return

    # /*
    #  * @brief Callback invoked when the Front Right slider is adjusted.
    #  */
    def on_slider_front_right_released(self):
        value = self.ui.Control_Front_Motor_Control_Slider_Right.value()
        self.app_log(f"CONTROL: Front Right Motor adjusted to {value}%")
        SysInfo("CONTROL: Front Right Motor adjusted to %d%%", value)
        self.motor_states["Front"]["R"] = value
        self._transmit_motor_update("Front", update_l=False, update_r=True)
        
        # /* Return from execution */
        return

    # /*
    #  * @brief Callback invoked when the Back Left slider is adjusted.
    #  */
    def on_slider_back_left_released(self):
        value = self.ui.Control_Back_Motor_Control_Slider_Left.value()
        self.app_log(f"CONTROL: Back Left Motor adjusted to {value}%")
        SysInfo("CONTROL: Back Left Motor adjusted to %d%%", value)
        self.motor_states["Back"]["L"] = value
        self._transmit_motor_update("Back", update_l=True, update_r=False)
        
        # /* Return from execution */
        return

    # /*
    #  * @brief Callback invoked when the Back Right slider is adjusted.
    #  */
    def on_slider_back_right_released(self):
        value = self.ui.Control_Back_Motor_Control_Slider_Right.value()
        self.app_log(f"CONTROL: Back Right Motor adjusted to {value}%")
        SysInfo("CONTROL: Back Right Motor adjusted to %d%%", value)
        self.motor_states["Back"]["R"] = value
        self._transmit_motor_update("Back", update_l=False, update_r=True)
        
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

    # /*
    #  * @brief Intercepts global key presses for application shortcuts.
    #  * @param event The QKeyEvent triggered by the OS.
    #  */
    def keyPressEvent(self, event):
        # /* Check if the Tab key was pressed to toggle views */
        if event.key() == Qt.Key_Tab:
            self.toggle_view()
            # /* Terminate event handling */
            return
            
        # /* Check if Control modifier is held down */
        if event.modifiers() & Qt.ControlModifier:
            # /* Trigger application exit on Ctrl+C or Ctrl+Z */
            if event.key() == Qt.Key_C or event.key() == Qt.Key_Z:
                self.app_log("Keyboard Interrupt: Exiting application...")
                SysInfo("Keyboard Interrupt (Ctrl+C / Ctrl+Z) detected. Closing app.")
                self.close()
                # /* Terminate event handling */
                return
                
        # /* Pass unhandled events to the parent class */
        super().keyPressEvent(event)
        
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
        
        # /* Scale the StackedWidget instead of just the inner Graph */
        new_stack_width = max(200, win_w - 670 - 20)
        new_stack_height = max(200, win_h - 40 - 40)
        self.ui.stackedWidget.setGeometry(670, 40, new_stack_width, new_stack_height)
        
        # /* Scale the GPS Map QWebEngineView to fill the StackedWidget */
        self.ui.Graph_ECUStatus_GPSMap.setGeometry(0, 0, new_stack_width, new_stack_height)
        
        # /* Scale the Visualization GroupBox height */
        vis_geom = self.ui.Visualization.geometry()
        new_vis_height = max(481, win_h - 73)
        self.ui.Visualization.setGeometry(vis_geom.x(), vis_geom.y(), vis_geom.width(), new_vis_height)
        
        # /* Check if log_console exists before resizing to prevent errors */
        if hasattr(self, 'log_console'):
            log_geom = self.log_console.geometry()
            
            # /* Reserve approx 40px for the control box at the bottom */
            button_space = 40
            new_log_height = max(101, new_vis_height - log_geom.y() - 20 - button_space)
            self.log_console.setGeometry(log_geom.x(), log_geom.y(), log_geom.width(), new_log_height)
            
            # /* Calculate new Y position for the Status Control Box */
            box_y = log_geom.y() + new_log_height + 5
            box_x = log_geom.x()
            
            # /* Dynamically reposition the entire Status Control Box if it exists */
            if hasattr(self.ui, 'Visualization_MonitorApp_StatusControl_Box'):
                ctrl_box = self.ui.Visualization_MonitorApp_StatusControl_Box
                box_geom = ctrl_box.geometry()
                ctrl_box.setGeometry(box_x, box_y, box_geom.width(), box_geom.height())
            # /* Fallback in case of typo in UI file */
            elif hasattr(self.ui, 'Visualization_MinitorApp_StatusControl_Box'):
                ctrl_box = self.ui.Visualization_MinitorApp_StatusControl_Box
                box_geom = ctrl_box.geometry()
                ctrl_box.setGeometry(box_x, box_y, box_geom.width(), box_geom.height())
        
        # /* Return from execution */
        return

# /* ========================================================================= */
# /* EXECUTION ENTRY POINT                                                     */
# /* ========================================================================= */

# /* Evaluate if the script is the main entry execution */
if __name__ == "__main__":
    SysInfo("Application execution started.")
    
    # /* Allow terminal Ctrl+C to terminate the PySide6 application gracefully */
    signal.signal(signal.SIGINT, signal.SIG_DFL)
    
    app = QApplication(sys.argv)
    window = MyMonitorApp()
    window.show()
    sys.exit(app.exec())