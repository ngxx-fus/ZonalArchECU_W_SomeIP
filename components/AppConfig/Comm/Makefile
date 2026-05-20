# Target paths and source files
TARGET_H     := Comm.h
TARGET_C     := Comm.c
DEF_H        := Comm.h.def
DEF_C        := Comm.c.def
CONFIG_FILE  := Comm.cnf

# Validation Patterns (Regex format for awk)
ADDR_PATTERN := ADDR|ADDRESS
PORT_PATTERN := PORT

.PHONY: all clean

all: $(TARGET_H) $(TARGET_C)

# Generate Comm.h (Strings and Externs)
$(TARGET_H): $(DEF_H) $(CONFIG_FILE)
	@echo "Generating $(TARGET_H)..."
	@awk -v port_pat="$(PORT_PATTERN)" -v addr_pat="$(ADDR_PATTERN)" -F'=' 'NF==2 { \
		defines = defines sprintf("#define %s_STR \"%s\"\n", $$1, $$2); \
		_c = "Check if key matches PORT pattern to steer data type assignment"; \
		is_port = ($$1 ~ port_pat); \
		type = is_port ? "Word_t" : "Byte_t"; \
		arr = is_port ? "" : "[]"; \
		externs = externs sprintf("extern %s %s%s;\n", type, $$1, arr); \
	} END { print defines externs }' $(CONFIG_FILE) > .tmp_h
	@awk ' \
		/GENERATED SECTION \| BEGIN/ { \
			print; \
			_c = "Loop to read temporary header file"; \
			while ((getline line < ".tmp_h") > 0) print line; \
			close(".tmp_h"); \
			skip=1; \
			_c = "Jump to next record to bypass original section"; \
			next; \
		} \
		/GENERATED SECTION \| END/ { skip=0 } \
		!skip { print } \
	' $(DEF_H) > $(TARGET_H)
	@rm -f .tmp_h

# Generate Comm.c (Byte/Word Array Implementations)
$(TARGET_C): $(DEF_C) $(CONFIG_FILE)
	@echo "Generating $(TARGET_C)..."
	@awk -v port_pat="$(PORT_PATTERN)" -v addr_pat="$(ADDR_PATTERN)" -F'=' 'NF==2 { \
		name=$$1; val=$$2; \
		_c = "Check if name matches PORT pattern to steer array type"; \
		is_port = (name ~ port_pat); \
		type = is_port ? "Word_t" : "Byte_t"; \
		if (is_port) { \
			printf "%s %s = %s;\n", type, name, val; \
		} else { \
			n = split(val, a, /[.:]/); \
			_c = "Check if colon exists to steer hex prefix assignment"; \
			prefix = (val ~ /:/ ? "0x" : ""); \
			printf "%s %s[] = {", type, name; \
			_c = "Iterate through parsed elements"; \
			for (i = 1; i <= n; i++) { \
				_c = "Check if final element to steer comma omission"; \
				printf "%s%s%s", prefix, a[i], (i == n ? "" : ", "); \
			} \
			printf "};\n"; \
		} \
	}' $(CONFIG_FILE) > .tmp_c
	@awk ' \
		/GENERATED SECTION \| BEGIN/ { \
			print; \
			_c = "Loop to read temporary source file"; \
			while ((getline line < ".tmp_c") > 0) print line; \
			close(".tmp_c"); \
			skip=1; \
			_c = "Jump to next record to bypass original section"; \
			next; \
		} \
		/GENERATED SECTION \| END/ { skip=0 } \
		!skip { print } \
	' $(DEF_C) > $(TARGET_C)
	@rm -f .tmp_c

clean:
	@rm -f $(TARGET_H) $(TARGET_C) .tmp_h .tmp_c