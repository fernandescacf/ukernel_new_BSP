BUILD_DIR = ${OUT_DIR}/${ARCH}
INCLUDES = -Iinclude -I${ROOT_DIR}/include -I${ROOT_DIR}/arch/include -I${ROOT_DIR}/arch/${ARCH}/mach/$(BOARD)

all: set_env boot cache mmu pmu
	$(LD) -r ${BUILD_DIR}/*.o -o ${OUT_DIR}/${TARGET}/arch.o

set_env:
	@echo 'Set build directory: ${BUILD_DIR}/'
	@if [ -d '${BUILD_DIR}' ]; then rm -rf "${BUILD_DIR}/*"; else mkdir "${BUILD_DIR}"; fi

boot:
	$(CC) $(CFLAGS) boot.S ${INCLUDES} -o ${BUILD_DIR}/boot.o

cache:
	$(CC) $(CFLAGS) cache.S ${INCLUDES} -o ${BUILD_DIR}/cache.o

mmu:
	$(CC) $(CFLAGS) mmu.c ${INCLUDES} -o ${BUILD_DIR}/mmu.o

pmu:
	$(CC) $(CFLAGS) pmu.S ${INCLUDES} -o ${BUILD_DIR}/pmu.o
