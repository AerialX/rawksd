TARGET := stripios.exe
$(TARGET): stripios.cpp rijndael.cpp
	@echo compiling $@ ...
	@$(CXX) -o $@ $^

clean:
	@echo "clean ..."
	@rm -f $(TARGET)
