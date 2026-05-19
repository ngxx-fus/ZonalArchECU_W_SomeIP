# Target paths and source files
TARGET_H    := Comm.h
TARGET_C    := Comm.c
DEF_H       := Comm.h.def
DEF_C       := Comm.c.def
CONFIG_FILE := Comm.cnf

.PHONY: all clean

all: $(TARGET_H) $(TARGET_C)

# Generate Comm.h (Strings and Externs)
$(TARGET_H): $(DEF_H) $(CONFIG_FILE)
	@echo "Generating $(TARGET_H)..."
	@awk -F'=' 'NF==2 { \
		defines = defines sprintf("#define %s_STR \"%s\"\n", $$1, $$2); \
		externs = externs sprintf("extern Byte_t %s[];\n", $$1); \
	} END { print defines externs }' $(CONFIG_FILE) > .tmp_h
	@awk ' \
		/GENERATED SECTION \| BEGIN/ { \
			print; while ((getline line < ".tmp_h") > 0) print line; \
			close(".tmp_h"); skip=1; next; \
		} \
		/GENERATED SECTION \| END/ { skip=0 } \
		!skip { print } \
	' $(DEF_H) > $(TARGET_H)
	@rm -f .tmp_h

# Generate Comm.c (Byte Array Implementations)
$(TARGET_C): $(DEF_C) $(CONFIG_FILE)
	@echo "Generating $(TARGET_C)..."
	@awk -F'=' 'NF==2 { \
		name=$$1; val=$$2; \
		n = split(val, a, /[.:]/); \
		prefix = (val ~ /:/ ? "0x" : ""); \
		printf "Byte_t %s[] = {", name; \
		for (i = 1; i <= n; i++) { \
			printf "%s%s%s", prefix, a[i], (i == n ? "" : ", "); \
		} \
		printf "};\n"; \
	}' $(CONFIG_FILE) > .tmp_c
	@awk ' \
		/GENERATED SECTION \| BEGIN/ { \
			print; while ((getline line < ".tmp_c") > 0) print line; \
			close(".tmp_c"); skip=1; next; \
		} \
		/GENERATED SECTION \| END/ { skip=0 } \
		!skip { print } \
	' $(DEF_C) > $(TARGET_C)
	@rm -f .tmp_c

clean:
	@rm -f $(TARGET_H) $(TARGET_C) .tmp_h .tmp_c