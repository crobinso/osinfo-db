
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

INTLTOOL_MERGE = intltool-merge
INTLTOOL_EXTRACT = intltool-extract
INTLTOOL_UPDATE = intltool-update

GETTEXT_PACKAGE = osinfo-db

SED = sed

DATA_FILES_IN = $(wildcard $(VPATH)/data/*/*/*.xml.in) $(wildcard $(VPATH)/data/*/*/*/*.xml.in)
DATA_FILES = $(DATA_FILES_IN:$(VPATH)/%.in=%)

SCHEMA_FILES_IN = data/schema/osinfo.rng.in
SCHEMA_FILES = data/schema/osinfo.rng

ARCHIVE = osinfo-db-$(TODAY).tar.xz

ZANATA = zanata-cli

XMLLINT = xmllint

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

INTLTOOL_MERGE_OPTS = $(INTLTOOL_MERGE_OPTS_$(V))
INTLTOOL_MERGE_OPTS_0 = -q
INTLTOOL_MERGE_OPTS_1 =


all: $(ARCHIVE) osinfo-db.spec mingw-osinfo-db.spec

install: $(ARCHIVE)
	$(OSINFO_DB_IMPORT) --root $(DESTDIR) $(OSINFO_DB_TARGET) $(ARCHIVE)

%.spec: %.spec.in Makefile
	$(V_GEN) $(SED) -e "s/@VERSION@/$(TODAY)/" < $< > $@

rpm: osinfo-db.spec $(ARCHIVE)
	rpmbuild --define "_sourcedir `pwd`" -ba osinfo-db.spec

mingwrpm:  mingw-osinfo-db.spec $(ARCHIVE)
	rpmbuild --define "_sourcedir `pwd`" -ba mingw-osinfo-db.spec


%.xml: %.xml.in Makefile
	@mkdir -p `dirname $@` po
	$(V_I18N) LC_ALL=C $(INTLTOOL_MERGE) $(INTLTOOL_MERGE_OPTS) -x -u -c po/.intltool-merge-cache $(VPATH)/po $< $@.tmp \
	    || { rm $@.tmp && exit 1; }
	@mv $@.tmp $@

%.rng: %.rng.in Makefile
	@mkdir -p `dirname $@` po
	$(V_GEN) $(SED) -e "s/@VERSION@/$(TODAY)/" < $< > $@

$(ARCHIVE): $(DATA_FILES) $(SCHEMA_FILES)
	$(V_EXP) $(OSINFO_DB_EXPORT) --license $(VPATH)/COPYING --version "$(TODAY)" --dir data $(ARCHIVE)

clean:
	rm -f osinfo-db-*.tar.xz
	rm -f $(DATA_FILES) $(SCHEMA_FILES) po/POTFILES.in po/osinfo-db.pot

po/POTFILES.in:
	$(V_GEN) find data -name *.xml.in | LC_ALL=C sort > $@

po/osinfo-db.pot: po/POTFILES.in $(DATA_FILES_IN)
	$(V_GEN) cd po && $(INTLTOOL_UPDATE) --gettext-package $(GETTEXT_PACKAGE) --pot

po-push: po/osinfo-db.pot
	$(ZANATA) push

po-pull: po/osinfo-db.pot
	$(ZANATA) pull

update-po:
	cd po && \
        for file in *.po; do \
	  lang=`echo $$file | $(SED) -e 's/.po//'`; \
          echo "$$lang:"; \
          result="`$(INTLTOOL_UPDATE) --gettext-package $(GETTEXT_PACKAGE) --dist -o $$lang.new.po $$lang`"; \
          if $$result; then \
            if cmp $$lang.po $$lang.new.po >/dev/null 2>&1; then \
              rm -f $$lang.new.po; \
            else \
              if mv -f $$lang.new.po $$lang.po; then \
                :; \
              else \
                echo "msgmerge for $$lang.po failed: cannot move $$lang.new.po to $$lang.po" 1>&2; \
                rm -f $$lang.new.po; \
                exit 1; \
              fi; \
            fi; \
          else \
            echo "msgmerge for $$lang.gmo failed!"; \
            rm -f $$lang.new.po; \
          fi; \
        done

check: $(DATA_FILES) $(SCHEMA_FILES)
	for xml in `find data -name '*.xml' | sort`; do \
	  if ! $(XMLLINT) --relaxng data/schema/osinfo.rng --noout $$xml; then \
	    exit 1; \
	  fi; \
	done

