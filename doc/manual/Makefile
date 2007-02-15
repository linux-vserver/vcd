# TODO: integrate with automake
# files
SRC := $(shell egrep -l '^[^%]*\\begin\{document\}' *.tex)
DST  = $(SRC:%.tex=%.pdf)

# commands
LATEX     = pdflatex
BIBTEX    = bibtex
MAKEINDEX = makeindex

# grep commands
RERUN = "(There were undefined references|Rerun to get (cross-references|the bars) right)"
RERUNBIB = "No file.*\.bbl|Citation.*undefined"
MAKEIDX = "^[^%]*\\makeindex"
MPRINT = "^[^%]*print"
USETHUMBS = "^[^%]*thumbpdf"


COPY = if test -r $(<:%.tex=%.toc); then \
	cp $(<:%.tex=%.toc) $(<:%.tex=%.toc.bak); \
fi

define run-latex
	$(COPY);$(LATEX) $<
	egrep $(MAKEIDX) $< && \
	($(MAKEINDEX) $(<:%.tex=%);$(COPY);$(LATEX) $<) >/dev/null; true
	egrep -c $(RERUNBIB) $(<:%.tex=%.log) && \
	($(BIBTEX) $(<:%.tex=%);$(COPY);$(LATEX) $<) ; true
	egrep $(RERUN) $(<:%.tex=%.log) && \
	($(COPY);$(LATEX) $<) >/dev/null; true
	egrep $(RERUN) $(<:%.tex=%.log) && \
	($(COPY);$(LATEX) $<) >/dev/null; true
	if cmp -s $(<:%.tex=%.toc) $(<:%.tex=%.toc.bak); then true; \
	else $(LATEX) $<; fi
	rm -f $(<:%.tex=%.toc.bak)
	egrep -i "(Reference|Citation).*undefined" $(<:%.tex=%.log); true
endef

all: $(DST)
	@touch $(SRC)

$(DST): $(SRC)
	@$(run-latex)

clean:
	-rm -f $(TARGET)
	-rm -f $(SRC:%.tex=%.aux) $(SRC:%.tex=%.bbl) $(SRC:%.tex=%.blg)
	-rm -f $(SRC:%.tex=%.log) $(SRC:%.tex=%.toc) $(SRC:%.tex=%.out)
	-rm -f $(SRC:%.tex=%.idx) $(SRC:%.tex=%.ilg) $(SRC:%.tex=%.ind)
