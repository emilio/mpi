TARGETS := report.pdf report-beamer.pdf report.html

.PHONY: all
all: $(TARGETS)
	@echo > /dev/null

.PHONY: clean
clean:
	$(RM) $(TARGETS)

%.pdf: %.md
	@$(info [DOC] $< -> $@)
	@pandoc $< --toc -f markdown -t latex --latex-engine=xelatex -o $@ \
					--filter ./latex-filter.py


%-beamer.pdf: %.md
	@$(info [DOC] $< -> $@)
	@pandoc $< --toc -f markdown -t beamer --latex-engine=xelatex -o $@ \
					-V theme=Malmoe                                             \
					-V colortheme=seahorse                                      \
					-V fonttheme=structuresmallcapsserif

%.html: %.md
	@$(info [DOC] $< -> $@)
	@pandoc -s $< --toc -f markdown -t revealjs -o $@ \
					-V revealjs-url=https://cdnjs.cloudflare.com/ajax/libs/reveal.js/3.2.0
