.PHONY: clean All

All:
	@echo "----------Building project:[ farftp - Debug ]----------"
	@cd "farftp" && "$(MAKE)" -f  "farftp.mk"
clean:
	@echo "----------Cleaning project:[ farftp - Debug ]----------"
	@cd "farftp" && "$(MAKE)" -f  "farftp.mk" clean
