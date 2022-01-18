BUILD_DIR = ${OUT_DIR}/lib
INCLUDES = -Iinclude -I${ROOT_DIR}/include -I${ROOT_DIR}/arch/include

all: set_env main
	@cp ${BUILD_DIR}/itoa.o ${OUT_DIR}/${TARGET}/

set_env:
	@echo 'Set build directory: ${BUILD_DIR}/'
	@if [ -d '${BUILD_DIR}' ]; then rm -rf "${BUILD_DIR}/*"; else mkdir "${BUILD_DIR}"; fi

main:
	$(CC) $(CFLAGS) itoa.c ${INCLUDES} -o ${BUILD_DIR}/itoa.o