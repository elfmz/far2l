.PHONY: clean All

All:
	@echo "----------Building project:[ WinPort - Debug ]----------"
	@cd "WinPort" && "$(MAKE)" -f  "WinPort.mk"
	@echo "----------Building project:[ far2l - Debug ]----------"
	@cd "far2l" && "$(MAKE)" -f  "far2l.mk"
clean:
	@echo "----------Cleaning project:[ WinPort - Debug ]----------"
	@cd "WinPort" && "$(MAKE)" -f  "WinPort.mk"  clean
	@echo "----------Cleaning project:[ far2l - Debug ]----------"
	@cd "far2l" && "$(MAKE)" -f  "far2l.mk" clean
