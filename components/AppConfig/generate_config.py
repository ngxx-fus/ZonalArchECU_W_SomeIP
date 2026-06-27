import yaml
import re
import sys
import os

def update_file(def_file, target_file, content, start_marker, stop_marker):
    if not def_file or def_file == "NONE":
        return
    if not os.path.exists(def_file):
        print(f"[*] Warning: Template {def_file} not found. Skipping.")
        return

    with open(def_file, 'r', encoding='utf-8') as f:
        template = f.read()

    start_idx = template.find(start_marker)
    stop_idx = template.find(stop_marker)

    if start_idx != -1 and stop_idx != -1 and start_idx < stop_idx:
        new_content = template[:start_idx + len(start_marker)] + "\n" + content + template[stop_idx:]
        
        # Ensure target directory exists
        os.makedirs(os.path.dirname(target_file), exist_ok=True)
        
        with open(target_file, 'w', encoding='utf-8') as f:
            f.write(new_content)
        print(f"[*] Generated {os.path.basename(target_file)} successfully.")
    else:
        print(f"[*] Warning: GENERATED SECTION markers not found or invalid in {def_file}")

def main():
    if len(sys.argv) < 2:
        print("Usage: python generate_config.py <SystemConfig.yml>")
        sys.exit(1)

    yml_path = sys.argv[1]
    with open(yml_path, 'r', encoding='utf-8') as f:
        config = yaml.safe_load(f)

    # Extract dynamic section markers
    general_cfg = config.get('General', {})
    start_marker = general_cfg.get('StartGeneratedSection', '/// GENERATED SECTION | BEGIN /////////////////////////////////////////////////')
    stop_marker = general_cfg.get('StopGeneratedSection', '/// GENERATED SECTION | END   /////////////////////////////////////////////////')

    comm_h_defines = ""
    comm_c_impls = ""
    pinout_h_defines = ""

    zecu_cfg = config.get('ZECU', {})

    def traverse(node, prefix=''):
        nonlocal comm_h_defines, comm_c_impls, pinout_h_defines
        
        if isinstance(node, dict):
            if 'Value' in node:
                val = str(node['Value'])
                if val == "NONE":
                    return
                
                dtype = node.get('Type', 'UInt')
                macro_name = node.get('Macro', prefix.strip('_').upper())

                # -- PINOUT --
                if "PINOUT" in prefix.upper():
                    # Format as Pin_t type for strict type checking
                    pinout_h_defines += f'#define {macro_name:<25} ((Pin_t){val})\n'
                    return

                # -- NETWORK & COMM --
                if dtype in ["IPv4", "IP"]:
                    comm_h_defines += f'#define {macro_name}_STR "{val}"\n'
                    parts = val.split('.')
                    if len(parts) == 4:
                        hex_val = f"0x{int(parts[3]):02X}{int(parts[2]):02X}{int(parts[1]):02X}{int(parts[0]):02X}"
                        comm_h_defines += f'#define {macro_name}_HEX {hex_val}\n'
                        comm_h_defines += f'#define {macro_name}_INITIALIZER {{{parts[0]}, {parts[1]}, {parts[2]}, {parts[3]}}}\n'
                        comm_h_defines += f'extern const Byte_t {macro_name}[];\n\n'
                        comm_c_impls += f'const Byte_t {macro_name}[] = {macro_name}_INITIALIZER;\n'

                elif dtype == "MAC":
                    comm_h_defines += f'#define {macro_name}_STR "{val}"\n'
                    mac_clean = val.replace(':', '').upper()
                    comm_h_defines += f'#define {macro_name}_HEX 0x{mac_clean}\n'
                    parts = val.split(':')
                    if len(parts) == 6:
                        comm_h_defines += f'#define {macro_name}_INITIALIZER {{0x{parts[0]}, 0x{parts[1]}, 0x{parts[2]}, 0x{parts[3]}, 0x{parts[4]}, 0x{parts[5]}}}\n'
                        comm_h_defines += f'extern const Byte_t {macro_name}[];\n\n'
                        comm_c_impls += f'const Byte_t {macro_name}[] = {macro_name}_INITIALIZER;\n'

                elif dtype in ["Port", "UInt"]:
                    comm_h_defines += f'#define {macro_name}_STR "{val}"\n'
                    comm_h_defines += f'#define {macro_name}_VAL {val}\n'
                    comm_h_defines += f'#define {macro_name}_HEX 0x{int(val):04X}\n'
                    comm_h_defines += f'extern const Word_t {macro_name};\n\n'
                    comm_c_impls += f'const Word_t {macro_name} = {val};\n'
                    
                elif dtype == "String":
                    comm_h_defines += f'#define {macro_name}_STR "{val}"\n\n'
                    
                # -- SERVICE ENUM MACRO --
                elif dtype == "Service":
                    # Generate macro for service enum value
                    comm_h_defines += f'#define {macro_name} {val}\n'
            else:
                for k, v in node.items():
                    new_prefix = f"{prefix}{k}_" if prefix else f"{k}_"
                    traverse(v, new_prefix)

    traverse(zecu_cfg, prefix='ZECU_')
    
    base_dir = os.path.dirname(yml_path)
    patch_path = config.get('PatchPath', {})

    # Apply Comm Patches
    comm_cfg = patch_path.get('Comm', {})
    if comm_cfg:
        folder = comm_cfg.get('folder', './Comm/')
        if comm_cfg.get('header_def', 'NONE') != 'NONE' and comm_cfg.get('header', 'NONE') != 'NONE':
            def_path = os.path.normpath(os.path.join(base_dir, folder, comm_cfg['header_def']))
            out_path = os.path.normpath(os.path.join(base_dir, folder, comm_cfg['header']))
            update_file(def_path, out_path, comm_h_defines, start_marker, stop_marker)
            
        if comm_cfg.get('source_def', 'NONE') != 'NONE' and comm_cfg.get('source', 'NONE') != 'NONE':
            def_path = os.path.normpath(os.path.join(base_dir, folder, comm_cfg['source_def']))
            out_path = os.path.normpath(os.path.join(base_dir, folder, comm_cfg['source']))
            update_file(def_path, out_path, comm_c_impls, start_marker, stop_marker)

    # Apply Pinout Patches
    pinout_cfg = patch_path.get('Pinout', {})
    if pinout_cfg:
        folder = pinout_cfg.get('folder', './Pinout/')
        if pinout_cfg.get('header_def', 'NONE') != 'NONE' and pinout_cfg.get('header', 'NONE') != 'NONE':
            def_path = os.path.normpath(os.path.join(base_dir, folder, pinout_cfg['header_def']))
            out_path = os.path.normpath(os.path.join(base_dir, folder, pinout_cfg['header']))
            update_file(def_path, out_path, pinout_h_defines, start_marker, stop_marker)

if __name__ == "__main__":
    main()