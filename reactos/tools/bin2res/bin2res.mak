BIN2RES_BASE = $(TOOLS_BASE_)bin2res
BIN2RES_BASE_ = $(BIN2RES_BASE)$(SEP)
BIN2RES_INT = $(INTERMEDIATE_)$(BIN2RES_BASE)
BIN2RES_INT_ = $(BIN2RES_INT)$(SEP)
BIN2RES_OUT = $(OUTPUT_)$(BIN2RES_BASE)
BIN2RES_OUT_ = $(BIN2RES_OUT)$(SEP)

$(BIN2RES_INT): | $(TOOLS_INT)
	$(ECHO_MKDIR)
	${mkdir} $@

ifneq ($(INTERMEDIATE),$(OUTPUT))
$(BIN2RES_OUT): | $(TOOLS_OUT)
	$(ECHO_MKDIR)
	${mkdir} $@
endif

BIN2RES_TARGET = \
	$(EXEPREFIX)$(BIN2RES_OUT_)bin2res$(EXEPOSTFIX)

BIN2RES_SOURCES = $(addprefix $(BIN2RES_BASE_), \
	bin2res.c \
	mkstemps.c \
	)

BIN2RES_OBJECTS = \
	$(addprefix $(INTERMEDIATE_), $(BIN2RES_SOURCES:.c=.o))

BIN2RES_HOST_CFLAGS = -Iinclude/reactos/wine -D__REACTOS__ $(TOOLS_CFLAGS)

BIN2RES_HOST_LFLAGS = $(TOOLS_LFLAGS)

$(BIN2RES_TARGET): $(BIN2RES_OBJECTS) | $(BIN2RES_OUT)
	$(ECHO_LD)
	${host_gcc} $(BIN2RES_OBJECTS) $(BIN2RES_HOST_LFLAGS) -o $@

$(BIN2RES_INT_)bin2res.o: $(BIN2RES_BASE_)bin2res.c | $(BIN2RES_INT)
	$(ECHO_CC)
	${host_gcc} $(BIN2RES_HOST_CFLAGS) -c $< -o $@

$(BIN2RES_INT_)mkstemps.o: $(BIN2RES_BASE_)mkstemps.c | $(BIN2RES_INT)
	$(ECHO_CC)
	${host_gcc} $(BIN2RES_HOST_CFLAGS) -c $< -o $@

.PHONY: bin2res
bin2res: $(BIN2RES_TARGET)

.PHONY: bin2res_clean
bin2res_clean:
	-@$(rm) $(BIN2RES_TARGET) $(BIN2RES_OBJECTS) 2>$(NUL)
clean: bin2res_clean
