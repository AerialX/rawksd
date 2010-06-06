all:
	@python kamek.py project.yaml
	@echo "Creating XML..."
	@cat riiv.template > riiv.xml
	@echo '<patch id="test_ntsc">' >> riiv.xml
	@cat build/ntsc_riiv.xml >> riiv.xml
	@echo '</patch>' >> riiv.xml
	@echo '<patch id="test_pal">' >> riiv.xml
	@cat build/pal_riiv.xml >> riiv.xml
	@echo '</patch>' >> riiv.xml
	@echo '</wiidisc>' >> riiv.xml
	
	@cat riiv_ntsc.template > ../data/rb2_ntsc.xml
	@echo '<patch id="kamek_ntsc">' >> ../data/rb2_ntsc.xml
	@cat build/ntsc_riiv.xml >> ../data/rb2_ntsc.xml
	@echo '</patch>' >> ../data/rb2_ntsc.xml
	@echo '</wiidisc>' >> ../data/rb2_ntsc.xml
	
	@cat riiv_pal.template > ../data/rb2_pal.xml
	@echo '<patch id="kamek_pal">' >> ../data/rb2_pal.xml
	@cat build/pal_riiv.xml >> ../data/rb2_pal.xml
	@echo '</patch>' >> ../data/rb2_pal.xml
	@echo '</wiidisc>' >> ../data/rb2_pal.xml

clean:
	rm -rf build
	rm -f riiv.xml
	rm -f ../data/rb2_ntsc.xml
	rm -f ../data/rb2_pal.xml
