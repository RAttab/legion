# ------------------------------------------------------------------------------
# Makefile
# Rémi Attab (remi.attab@gmail.com), 11 Jun 2023
# FreeBSD-style copyright and disclaimer apply
# ------------------------------------------------------------------------------

# -----------------------------------------------------------------------------
# config
# -----------------------------------------------------------------------------

PREFIX ?= build
TEST ?= ring lisp chunk lanes tech save protocol items proxy man

DEPS = opus alsa glfw3 opengl freetype2

CFLAGS := $(CFLAGS) -ggdb -O3 -march=native -pipe -std=gnu2x -D_GNU_SOURCE -lm -pthread
CFLAGS := $(CFLAGS) -Isrc -Isrc/engine
CFLAGS := $(CFLAGS) -DLEGION_BUILD_PREFIX="$(PREFIX)"
CFLAGS := $(CFLAGS) $(foreach dep,$(DEPS),$(shell pkg-config --cflags $(dep)))
CFLAGS := $(CFLAGS) -Wall -Wextra
CFLAGS := $(CFLAGS) -Wundef
CFLAGS := $(CFLAGS) -Wcast-align
CFLAGS := $(CFLAGS) -Wwrite-strings
CFLAGS := $(CFLAGS) -Wunreachable-code
CFLAGS := $(CFLAGS) -Wformat=2
CFLAGS := $(CFLAGS) -Winit-self
CFLAGS := $(CFLAGS) -Wno-format-truncation
CFLAGS := $(CFLAGS) -Wno-implicit-fallthrough
CFLAGS := $(CFLAGS) -Wno-address-of-packed-member

LIBS := $(foreach dep,$(DEPS),$(shell pkg-config --libs $(dep)))

VALGRIND_FLAGS := $(VALGRIND_FLAGS) --quiet
VALGRIND_FLAGS := $(VALGRIND_FLAGS) --leak-check=full
VALGRIND_FLAGS := $(VALGRIND_FLAGS) --track-origins=yes
VALGRIND_FLAGS := $(VALGRIND_FLAGS) --trace-children=yes
VALGRIND_FLAGS := $(VALGRIND_FLAGS) --error-exitcode=1
VALGRIND_FLAGS := $(VALGRIND_FLAGS) --suppressions=legion.supp

ASM := src/db/res.S

OBJECTS_GEN := gen common utils
OBJECTS_LEGION := common utils db vm items game ui ux engine

DB_OUTPUTS := $(wildcard src/db/gen/*.h) $(wildcard src/db/gen/*.S)
DB_INPUTS := res/io.lisp src/db/gen/tech.lisp $(wildcard res/stars/*.lisp)
DB_INPUTS := $(DB_INPUTS) $(wildcard res/man/*.lm)
DB_INPUTS := $(DB_INPUTS) $(wildcard res/man/asm/*.lm)
DB_INPUTS := $(DB_INPUTS) $(wildcard res/man/concepts/*.lm)
DB_INPUTS := $(DB_INPUTS) $(wildcard res/man/guides/*.lm)
DB_INPUTS := $(DB_INPUTS) $(wildcard res/man/items/*.lm)
DB_INPUTS := $(DB_INPUTS) $(wildcard res/man/lisp/*.lm)

SOUNDS := $(SOUNDS) $(wildcard res/sounds/*.wav)
SOUNDS := $(SOUNDS) $(wildcard res/sounds/*.opus)


# -----------------------------------------------------------------------------
# top
# -----------------------------------------------------------------------------

.PHONY: all
all: gen res legion test

.PHONY: clean
clean:
	@echo -e "\e[32m[clean]\e[0m"
	@rm -rf --preserve-root -- $(PREFIX)
	@mkdir -p $(PREFIX)/obj/
	@mkdir -p $(PREFIX)/test/

$(shell mkdir -p $(PREFIX)/obj/)
-include $(wildcard $(PREFIX)/obj/*.d)

$(PREFIX)/obj/%.o: src/%.c
	@echo -e "\e[32m[build]\e[0m $@"
	@$(CC) -MMD -MP -c -o $@ $< $(CFLAGS)

$(PREFIX)/obj/%-main.o: src/%/main.c
	@echo -e "\e[32m[build]\e[0m $@"
	@$(CC) -MMD -MP -c -o $@ $< $(CFLAGS)


# -----------------------------------------------------------------------------
# gen
# -----------------------------------------------------------------------------

$(PREFIX)/gen: $(PREFIX)/obj/gen-main.o $(foreach obj,$(OBJECTS_GEN),$(PREFIX)/obj/$(obj).o)
	@echo -e "\e[32m[build]\e[0m $@"
	@$(CC) -o $@ $^ $(CFLAGS)

src/db/gen/tech.lisp: res/tech.lisp $(PREFIX)/gen
	@echo -e "\e[32m[gen]\e[0m tech"
	@$(PREFIX)/gen --tech $< --src src/db/gen --output $(PREFIX) > $(PREFIX)/tech.log
	@cat $(PREFIX)/tech.dot | dot -Tsvg > $(PREFIX)/tech.svg

$(DB_OUTPUTS) &: $(DB_INPUTS) $(PREFIX)/gen
	@echo -e "\e[32m[gen]\e[0m db"
	@$(PREFIX)/gen --db res --src src/db/gen

.PHONY: gen-tech gen-db
ifneq ($(SKIP_GEN),1)
gen-tech: src/db/gen/tech.lisp
gen-db: $(DB_OUTPUTS)
else
gen-tech:
	@echo -e "\e[32m[gen]\e[0m skip $@"
gen-db:
	@echo -e "\e[32m[gen]\e[0m skip $@"
endif

.PHONY: gen
gen: $(PREFIX)/gen gen-tech gen-db


# -----------------------------------------------------------------------------
# res
# -----------------------------------------------------------------------------

$(shell mkdir -p $(PREFIX)/sounds/)
OPUS := $(foreach sound, $(SOUNDS), $(PREFIX)/sounds/$(basename $(notdir $(sound))).opus)

.PHONY: sounds
sounds: $(OPUS)

$(PREFIX)/sounds/%.opus: res/sounds/%.*
	@echo -e "\e[32m[sound]\e[0m $@"
	@ffmpeg -y -v error -i $< -strict -2 -acodec opus -ac 2 -ar 48000 $@


$(shell mkdir -p $(PREFIX)/shaders/)
SHADERS := $(foreach shader,$(wildcard res/shaders/*),$(PREFIX)/shaders/$(notdir $(shader)).glsl)
shaders: $(SHADERS)

$(PREFIX)/shaders/%.glsl: res/shaders/%
	@echo -e "\e[32m[shader]\e[0m $<"
	@glslang --quiet --glsl-version 430 -l $<
	@touch $@


src/db/res.S: src/db/gen/man.S
src/db/res.S: $(wildcard res/img/*.bmp)
src/db/res.S: $(wildcard res/font/*.otf)
src/db/res.S: $(OPUS) $(SHADERS)
	@echo -e "\e[32m[res]\e[0m $@"
	@touch $@


# -----------------------------------------------------------------------------
# legion
# -----------------------------------------------------------------------------

$(PREFIX)/liblegion.a: $(foreach obj,$(OBJECTS_LEGION),$(PREFIX)/obj/$(obj).o)
	@echo -e "\e[32m[ar]\e[0m $@"
	@ar rcs $@ $^

$(PREFIX)/legion: $(PREFIX)/obj/legion-main.o $(PREFIX)/obj/legion.o $(PREFIX)/liblegion.a $(ASM)
	@echo -e "\e[32m[build]\e[0m $@"
	@$(CC) -o $@ $^ $(LIBS) $(CFLAGS)

.PHONY: legion
legion: $(PREFIX)/legion

.PHONY: run
run: $(PREFIX)/legion
	@echo -e "\e[32m[run]\e[0m $<"
	@$(PREFIX)/legion

.PHONY: run-metrics
run-metrics: $(PREFIX)/legion
	@echo -e "\e[32m[run]\e[0m $<"
	@$(PREFIX)/legion -m $(PREFIX)/metrics.lisp


# -----------------------------------------------------------------------------
# test
# -----------------------------------------------------------------------------

$(shell mkdir -p $(PREFIX)/test/)

.PRECIOUS: $(foreach test,$(TEST),$(PREFIX)/obj/test-$(test).o)
$(PREFIX)/obj/test-%.o: test/%_test.c
	@echo -e "\e[32m[build]\e[0m $@"
	@$(CC) -MMD -MP -c -o $@ $< $(CFLAGS)

.PRECIOUS: $(foreach test,$(TEST),$(PREFIX)/test/$(test))
$(PREFIX)/test/%: $(PREFIX)/obj/test-%.o $(PREFIX)/liblegion.a $(ASM)
	@echo -e "\e[32m[build]\e[0m $@"
	@$(CC) -o $@ $^ $(LIBS) $(CFLAGS)

test-%: $(PREFIX)/test/%
	@echo -e "\e[32m[test]\e[0m $@"
	@$< $(PREFIX)

.PHONY: test
test: $(foreach test,$(TEST),test-$(test))

valgrind-%: $(PREFIX)/test/%
	@echo -e "\e[32m[test]\e[0m $@"
	@valgrind $(VALGRIND_FLAGS) $<

valgrind-run: $(PREFIX)/legion
	@echo -e "\e[32m[run]\e[0m $@"
	@valgrind $(VALGRIND_FLAGS) $(PREFIX)/legion

.PHONY: valgrind
valgrind: $(foreach test,$(TEST),valgrind-$(test))
