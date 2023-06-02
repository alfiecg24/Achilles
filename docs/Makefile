FILES=ExploitWriteup VulnerabilityWriteup
MDS=$(foreach file,$(FILES),$(file).md)

# check if mdpdf is installed
ifeq (, $(shell which mdpdf))
$(error "No mdpdf in $(PATH), consider running `npm install -g mdpdf`")
endif

all: pdf

pdf: $(MDS)
	@for file in $(MDS); do \
		echo "Generating PDF for $$file"; \
		mdpdf $$file; \
	done
	@echo "Done"