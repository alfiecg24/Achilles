FILES=ExploitWriteup VulnerabilityWriteup
MDS=$(foreach file,$(FILES),$(file).md)

# check if mdpdf is installed
ifeq (, $(shell which mdpdf))
$(error "Generating the PDFs requires mdpdf to be installed, consider running `npm install -g mdpdf` to do so.")
endif

all: docs

docs: $(MDS)
	@for file in $(MDS); do \
		echo "Generating PDF for $$file"; \
		mdpdf $$file; \
	done
	@echo "Done"