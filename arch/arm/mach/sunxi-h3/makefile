BUILD_DIR = ${OUT_DIR}/$(BOARD)
INCLUDES = -Iinclude -I${ROOT_DIR}/include -I${ROOT_DIR}/arch/include

all: set_env serial
	@cp ${BUILD_DIR}/serial.o ${OUT_DIR}/${TARGET}/

set_env:
	@echo 'Set build directory: ${BUILD_DIR}/'
	@if [ -d '${BUILD_DIR}' ]; then rm -rf "${BUILD_DIR}/*"; else mkdir "${BUILD_DIR}"; fi

serial:
	$(CC) $(CFLAGS) uart.c ${INCLUDES} -o ${BUILD_DIR}/serial.o