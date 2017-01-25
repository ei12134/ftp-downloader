
tex=report.tex
pdf=report.pdf
res=network1.png


.PHONY: clean view
.PHONY: ${res}


${pdf}: ${tex} ${res}
	pdflatex ${tex}

view: ${pdf}
	zathura ${pdf}

clean:
	rm -f *.log *.aux *.dvi *.pdf

