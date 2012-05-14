pdf: progress_report final_report 

progress_report: progress_report.tex 
	pdflatex progress_report.tex
	rm *.log *.aux

final_report: final_report.tex
	pdflatex final_report.tex
	rm *.log *.aux

clean:
	rm *.pdf

