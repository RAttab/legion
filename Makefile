# ------------------------------------------------------------------------------
# Makefile
# Rémi Attab (remi.attab@gmail.com), 11 Jun 2023
# FreeBSD-style copyright and disclaimer apply
# ------------------------------------------------------------------------------

# -----------------------------------------------------------------------------
# config
# -----------------------------------------------------------------------------

PREFIX ?= build

TEST ?= ring text lisp chunk lanes tech save protocol items proxy man

RES := font/*.otf gen/*.lisp img/*.bmp mods/*.lisp
RES := $(RES) man/*.lm
RES := $(RES) man/asm/*.lm
RES := $(RES) man/concepts/*.lm
RES := $(RES) man/guides/*.lm
RES := $(RES) man/items/*.lm
RES := $(RES) man/lisp/*.lm
RES := $(RES) man/sys/*.lm

OBJECTS_LEGION = common items ui render game vm utils db
OBJECTS_GEN = gen common utils

CFLAGS := $(CFLAGS) -ggdb -O3 -march=native -pipe -std=gnu11 -D_GNU_SOURCE -lm -pthread
CFLAGS := $(CFLAGS) -Isrc
CFLAGS := $(CFLAGS) $(shell sdl2-config --cflags)
CFLAGS := $(CFLAGS) $(shell pkg-config --cflags freetype2)
CFLAGS := $(CFLAGS) -Wall -Wextra
CFLAGS := $(CFLAGS) -Wundef
CFLAGS := $(CFLAGS) -Wcast-align
CFLAGS := $(CFLAGS) -Wwrite-strings
CFLAGS := $(CFLAGS) -Wunreachable-code
CFLAGS := $(CFLAGS) -Wformat=2
CFLAGS := $(CFLAGS) -Winit-self
CFLAGS := $(CFLAGS) -Wno-implicit-fallthrough
CFLAGS := $(CFLAGS) -Wno-address-of-packed-member

LIBS := $(LIBS) $(shell sdl2-config --libs)
LIBS := $(LIBS) $(shell pkg-config --libs freetype2)


# -----------------------------------------------------------------------------
# top
# -----------------------------------------------------------------------------

.PHONY: all
all: gen legion test res

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

GEN_DB_FILES := $(wildcard src/db/gen/*.h)
$(GEN_DB_FILES) &: res/io.lisp src/db/gen/tech.lisp $(PREFIX)/gen
	@echo -e "\e[32m[gen]\e[0m db"
	@$(PREFIX)/gen --db res --src src/db/gen

.PHONY: gen-tech gen-db
ifneq ($(SKIP_GEN),1)
gen-tech: src/db/gen/tech.lisp
gen-db: $(GEN_DB_FILES)
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

$(PREFIX)/res/%: res/%
	@echo -e "\e[32m[res]\e[0m $@"
	@mkdir -p $(dir $@)
	@cp -r $< $@

.PHONY: res
res: $(foreach res,$(RES),$(foreach path,$(wildcard res/$(res)),$(PREFIX)/$(path)))


# -----------------------------------------------------------------------------
# legion
# -----------------------------------------------------------------------------

$(PREFIX)/liblegion.a: $(foreach obj,$(OBJECTS_LEGION),$(PREFIX)/obj/$(obj).o)
	@echo -e "\e[32m[ar]\e[0m $@"
	@ar rcs $@ $^

$(PREFIX)/legion: $(PREFIX)/obj/legion-main.o $(PREFIX)/obj/legion.o $(PREFIX)/liblegion.a
	@echo -e "\e[32m[build]\e[0m $@"
	@$(CC) -o $@ $^ $(LIBS) $(CFLAGS)

legion: $(PREFIX)/legion

run: $(PREFIX)/legion res
	@echo -e "\e[32m[run]\e[0m $<"
	@$(PREFIX)/legion

# -----------------------------------------------------------------------------
# test
# -----------------------------------------------------------------------------

$(shell mkdir -p $(PREFIX)/test/)

.PRECIOUS: $(foreach test,$(TEST),$(PREFIX)/obj/test-$(test).o)
$(PREFIX)/obj/test-%.o: test/%_test.c
	@echo -e "\e[32m[build]\e[0m $@"
	@$(CC) -MMD -MP -c -o $@ $< $(CFLAGS)

.PRECIOUS: $(foreach test,$(TEST),$(PREFIX)/test/$(test))
$(PREFIX)/test/%: $(PREFIX)/obj/test-%.o $(PREFIX)/liblegion.a
	@echo -e "\e[32m[build]\e[0m $@"
	@$(CC) -o $@ $^ $(LIBS) $(CFLAGS)

test-%: $(PREFIX)/test/% res
	@echo -e "\e[32m[test]\e[0m $@"
	@$< $(PREFIX)

.PHONY: test
test: $(foreach test,$(TEST),test-$(test))

valgrind-%: $(PREFIX)/test/% res
	@echo -e "\e[32m[test]\e[0m $@"
	@valgrind \
	        --quiet \
	        --leak-check=full \
	        --track-origins=yes \
	        --trace-children=yes \
	        --error-exitcode=1 \
	        --suppressions=legion.supp \
		$<

.PHONY: valgrind
valgrind: $(foreach test,$(TEST),valgrind-$(test))
