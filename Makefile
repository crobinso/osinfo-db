VPATH = .

ifdef SOURCE_DATE_EPOCH
    TODAY = $(shell date --utc --date="@$(SOURCE_DATE_EPOCH)" +"%Y%m%d")
else
    TODAY = $(shell date +"%Y%m%d")
endif

OSINFO_DB_EXPORT = osinfo-db-export
OSINFO_DB_IMPORT = osinfo-db-import

DESTDIR = /
OSINFO_DB_TARGET = --user

XGETTEXT = xgettext
MSGMERGE = msgmerge
MSGFMT = msgfmt

GETTEXT_PACKAGE = osinfo-db

SED = sed

DATA_FILES_IN = $(wildcard $(VPATH)/data/*/*/*.xml.in) $(wildcard $(VPATH)/data/*/*/*/*.xml.in)
DATA_FILES = $(DATA_FILES_IN:$(VPATH)/%.in=%)

SCHEMA_FILES_IN = data/schema/osinfo.rng.in
SCHEMA_FILES = data/schema/osinfo.rng

SPEC_FILES_IN = osinfo-db.spec.in
SPEC_FILES = $(SPEC_FILES_IN:%.in=%)

PO_FILES = $(wildcard $(VPATH)/po/*.po)
ITS_RULES = $(VPATH)/po/gettext/its/osinfo-db.its

ARCHIVE = osinfo-db-$(TODAY).tar.xz

PYTHON = python3

V = 0

V_I18N = $(V_I18N_$(V))
V_I18N_0 = @echo "  I18N     " $@;
V_I18N_1 =

V_GEN = $(V_GEN_$(V))
V_GEN_0 = @echo "  GEN      " $@;
V_GEN_1 =

V_EXP = $(V_EXP_$(V))
V_EXP_0 = @echo "  EXP      " $@;
V_EXP_1 =

ABS_TOPDIR = $(dir $(abspath $(lastword $(MAKEFILE_LIST))))

all: $(ARCHIVE) $(SPEC_FILES)

install: $(ARCHIVE)
	$(OSINFO_DB_IMPORT) --root $(DESTDIR) $(OSINFO_DB_TARGET) $(ARCHIVE)

%.spec: %.spec.in Makefile
	$(V_GEN) $(SED) -e "s/@VERSION@/$(TODAY)/" < $< > $@

rpm: osinfo-db.spec $(ARCHIVE)
	rpmbuild --define "_sourcedir `pwd`" -ba osinfo-db.spec


%.xml: %.xml.in Makefile $(PO_FILES)
	@mkdir -p `dirname $@`
	$(V_I18N) env XDG_DATA_DIRS=$(VPATH)/po $(MSGFMT) --xml --template $< -d $(VPATH)/po -o $@

%.rng: %.rng.in Makefile
	@mkdir -p `dirname $@`
	$(V_GEN) $(SED) -e "s/@VERSION@/$(TODAY)/" < $< > $@

$(ARCHIVE): $(DATA_FILES) $(SCHEMA_FILES)
	$(V_EXP) $(OSINFO_DB_EXPORT) --license $(VPATH)/COPYING --version "$(TODAY)" --dir data $(ARCHIVE)

clean:
	rm -f osinfo-db-*.tar.xz
	rm -f $(DATA_FILES) $(SCHEMA_FILES) $(SPEC_FILES)

po/osinfo-db.pot: $(DATA_FILES_IN)
	$(V_GEN) $(XGETTEXT) --its $(ITS_RULES) -F -o $@ --package-name $(GETTEXT_PACKAGE) $(DATA_FILES_IN)

update-po:
	cd po && \
        for file in *.po; do \
	  lang=`basename $$file .po`; \
          echo "$$lang:"; \
          if ! $(MSGMERGE) --previous -o $$file $$file osinfo-db.pot; then \
            echo "msgmerge for $$file failed!"; \
          fi; \
        done

check: $(DATA_FILES) $(SCHEMA_FILES)
	INTERNAL_OSINFO_DB_DATA_DIR=data INTERNAL_OSINFO_DB_DATA_SRC_DIR=$(ABS_TOPDIR)data $(PYTHON) -m pytest $(ABS_TOPDIR)tests
