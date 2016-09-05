.PHONY: clean All

All:
	@echo "----------Building project:[ WinPort - Debug ]----------"
	@cd "WinPort" && "$(MAKE)" -f  "WinPort.mk"
	@echo "----------Building project:[ farlng - Debug ]----------"
	@cd "farlng" && "$(MAKE)" -f  "farlng.mk"
	@echo "----------Building project:[ far2l - Debug ]----------"
	@cd "far2l" && "$(MAKE)" -f  "far2l.mk" PreBuild && "$(MAKE)" -f  "far2l.mk" && "$(MAKE)" -f  "far2l.mk" PostBuild
	@echo "----------Building project:[ colorer - Debug ]----------"
	@cd "colorer" && "$(MAKE)" -f  "colorer.mk" PreBuild && "$(MAKE)" -f  "colorer.mk" && "$(MAKE)" -f  "colorer.mk" PostBuild
	@echo "----------Building project:[ farftp - Debug ]----------"
	@cd "farftp" && "$(MAKE)" -f  "farftp.mk" PreBuild && "$(MAKE)" -f  "farftp.mk" && "$(MAKE)" -f  "farftp.mk" PostBuild
	@echo "----------Building project:[ multiarc - Debug ]----------"
	@cd "multiarc" && "$(MAKE)" -f  "multiarc.mk" PreBuild && "$(MAKE)" -f  "multiarc.mk"
	@echo "----------Building project:[ tmppanel - Debug ]----------"
	@cd "tmppanel" && "$(MAKE)" -f  "tmppanel.mk" PreBuild && "$(MAKE)" -f  "tmppanel.mk"
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
	@echo "----------Cleaning project:[ multiarc - Debug ]----------"
	@cd "multiarc" && "$(MAKE)" -f  "multiarc.mk"  clean
	@echo "----------Cleaning project:[ tmppanel - Debug ]----------"
	@cd "tmppanel" && "$(MAKE)" -f  "tmppanel.mk"  clean
	@echo "----------Cleaning project:[ _All - Debug ]----------"
	@cd "_All" && "$(MAKE)" -f  "_All.mk" clean
