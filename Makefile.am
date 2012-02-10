SUBDIRS = rest rest-extras examples docs tests

%-$(API_VERSION).pc: %.pc
	$(AM_V_GEN)cp $< $@

pkgconfigdir   = $(libdir)/pkgconfig
pkgconfig_DATA = rest-$(API_VERSION).pc rest-extras-$(API_VERSION).pc

CLEANFILES = $(pkgconfig_DATA)

DISTCHECK_CONFIGURE_FLAGS = --enable-gtk-doc

ACLOCAL_AMFLAGS = -I build ${ACLOCAL_FLAGS}

all-local: rest-$(API_VERSION).pc rest-extras-$(API_VERSION).pc

if GCOV_ENABLED
cov-reset:
	@rm -fr coverage
	@find . -name "*.gcda" -exec rm {} \;
	@lcov --directory . --zerocounters

cov-report:
	@mkdir -p coverage
	@lcov --compat-libtool --directory . --capture --output-file coverage/app.info
	@genhtml -o coverage/ coverage/app.info

cov:
	@make cov-report

clean-local:
	@make cov-reset

check:
	@make cov
endif
