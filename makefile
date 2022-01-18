ifndef BOARD_CONFIG
	ifneq ($(MAKECMDGOALS),clean)
		$(error No board configuration selected)
	endif
else
	include config/${BOARD_CONFIG}
endif

ifeq ($(MAKECMDGOALS),release)
	CFLAGS += -O2
	BUILD_MODE = release
else
	CFLAGS += -g
	BUILD_MODE = debug
endif

ifdef EARLY_UART
	CFLAGS += -DUSE_EARLY_UART
endif

LD_DIR = arch/$(ARCH)

ROOT_DIR := $(shell dirname $(realpath $(firstword $(MAKEFILE_LIST))))
OUT_DIR = ${ROOT_DIR}/build
BIN_DIR = ${ROOT_DIR}/bin

ifndef TARGET
	TARGET = ${BOARD}-ukernel
endif

export CFLAGS
export CC
export LD
export ARCH
export BOARD
export ROOT_DIR
export OUT_DIR
export BIN_DIR
export TARGET

.PHONY: debug
.PHONY: release
.PHONY: clean
.PHONY: arch
.PHONY: board
.PHONY: init
.PHONY: lib
.PHONY: bin

release: all

debug: all

all: info set_env arch board init lib $(TARGET) bin

info:
	@echo 'Bare Metal OS build started with:'
	@echo '  Board: ${BOARD}'
	@echo '  Board config: ${BOARD_CONFIG}'
	@echo '  Architecture: ${ARCH} ${CPU}'
	@echo '  Build type: ${BUILD_MODE}'
	@echo '  Target name: ${TARGET}'

set_env:
	@echo 'Set up build directories: '
	@echo '  ${OUT_DIR}/'
	@if [ -d '${OUT_DIR}' ]; then rm -rf ${OUT_DIR}/*; else mkdir "${OUT_DIR}"; fi
	@echo '  ${OUT_DIR}/${TARGET}'
	@if [ -d '${OUT_DIR}/${TARGET}' ]; then rm -rf ${OUT_DIR}/${TARGET}/*; else mkdir "${OUT_DIR}/${TARGET}"; fi
	@echo '  ${BIN_DIR}/'
	@if [ -d '${BIN_DIR}' ]; then rm -rf ${BIN_DIR}/*; else mkdir "${BIN_DIR}"; fi

arch:
	$(MAKE) -C arch/$(ARCH)

board:
	$(MAKE) -C arch/$(ARCH)/mach/$(BOARD)

init:
	$(MAKE) -C init

ipc:
	$(MAKE) -C ipc/

kernel:
	$(MAKE) -C kernel/

lib:
	$(MAKE) -C lib/

loader:
	$(MAKE) -C loader/

memory:
	$(MAKE) -C memory/

sync:
	$(MAKE) -C sync/

system:
	$(MAKE) -C system/

virtual:
	$(MAKE) -C virtual/

$(TARGET):
	$(CC) -nostartfiles -T $(LD_DIR)/lscript.ld \
	${OUT_DIR}/${TARGET}/*.o -o ${BIN_DIR}/$(TARGET).elf
	
bin:
	$(COMPILER)-objcopy -O binary ${BIN_DIR}/$(TARGET).elf ${BIN_DIR}/$(TARGET).bin
	@echo '*** Finished building ***'
	@echo '$(TARGET) section sizes:'
	@$(COMPILER)-size bin/$(TARGET).elf
	@echo -n '$(TARGET).bin size: ' && wc -c < ${BIN_DIR}/$(TARGET).bin

clean:
	@if [ -d '${OUT_DIR}' ]; then rm -rf ${OUT_DIR}/; fi
	@if [ -d '${BIN_DIR}' ]; then rm -rf ${BIN_DIR}/; fi
	@echo 'Clean done'
