.PHONY: clean All

All:
	@echo "----------Building project:[ WinPort - Debug ]----------"
	@cd "WinPort" && "$(MAKE)" -f  "WinPort.mk"
	@echo "----------Building project:[ farlng - Debug ]----------"
	@cd "farlng" && "$(MAKE)" -f  "farlng.mk"
	@echo "----------Building project:[ far2l - Debug ]----------"
	@cd "far2l" && "$(MAKE)" -f  "far2l.mk"
	@echo "----------Building project:[ colorer - Debug ]----------"
	@cd "colorer" && "$(MAKE)" -f  "colorer.mk"
	@echo "----------Building project:[ farftp - Debug ]----------"
	@cd "farftp" && "$(MAKE)" -f  "farftp.mk"
	@echo "----------Building project:[ _All - Debug ]----------"
	@cd "_All" && "$(MAKE)" -f  "_All.mk"
clean:
	@echo "----------Cleaning project:[ WinPort - Debug ]----------"
	@cd "WinPort" && "$(MAKE)" -f  "WinPort.mk"  clean
	@echo "----------Cleaning project:[ farlng - Debug ]----------"
	@cd "farlng" && "$(MAKE)" -f  "farlng.mk"  clean
	@echo "----------Cleaning project:[ far2l - Debug ]----------"
	@cd "far2l" && "$(MAKE)" -f  "far2l.mk"  clean
	@echo "----------Cleaning project:[ colorer - Debug ]----------"
	@cd "colorer" && "$(MAKE)" -f  "colorer.mk"  clean
	@echo "----------Cleaning project:[ farftp - Debug ]----------"
	@cd "farftp" && "$(MAKE)" -f  "farftp.mk"  clean
	@echo "----------Cleaning project:[ _All - Debug ]----------"
	@cd "_All" && "$(MAKE)" -f  "_All.mk" clean
