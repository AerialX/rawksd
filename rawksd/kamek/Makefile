all:
	python kamek.py project.yaml
	@echo "Creating XML..."
	@cat riiv.template > riiv.xml
	@echo '<patch id="test_ntsc">' >> riiv.xml
	@cat build/ntsc_riiv.xml >> riiv.xml
	@echo '</patch>' >> riiv.xml
	@echo '<patch id="test_pal">' >> riiv.xml
	@cat build/pal_riiv.xml >> riiv.xml
	@echo '</patch>' >> riiv.xml
	@echo '</wiidisc>' >> riiv.xml

