# Target paths and source files
TARGET_FILE := Pinout.h
DEF_FILE    := Pinout.h.def
CONFIG_FILE := Pinout.cnf

.PHONY: all clean

all: $(TARGET_FILE)

$(TARGET_FILE): $(DEF_FILE) $(CONFIG_FILE)
	@echo "Generating $(TARGET_FILE)..."
	@awk -F'=' 'NF==2 {printf "#define %s ((Pin_t)%s)\n", $$1, $$2}' $(CONFIG_FILE) > .tmp_pins
	@awk ' \
		/GENERATED SECTION \| BEGIN/ { \
			print; \
			while ((getline line < ".tmp_pins") > 0) print line; \
			close(".tmp_pins"); \
			skip=1; \
			next; \
		} \
		/GENERATED SECTION \| END/ { skip=0 } \
		!skip { print } \
	' $(DEF_FILE) > $(TARGET_FILE)
	@rm -f .tmp_pins
	@echo "Code generation completed!"

clean:
	@rm -f $(TARGET_FILE) .tmp_pins