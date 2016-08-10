.PHONY: clean All

All:
	@echo "----------Building project:[ colorer - Debug ]----------"
	@cd "colorer" && "$(MAKE)" -f  "colorer.mk"
clean:
	@echo "----------Cleaning project:[ colorer - Debug ]----------"
	@cd "colorer" && "$(MAKE)" -f  "colorer.mk" clean
