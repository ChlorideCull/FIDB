files = database.cpp
soname = fuckit

build-shared:
	clang -O2 -fPIC -shared -o lib$(soname).so $(files)
	strip --strip-unneeded lib$(soname).so

build-tiny:
	clang -Oz -fPIC -shared -o lib$(soname).so $(files)
	strip --strip-unneeded lib$(soname).so

build-debug:
	clang -O0 -g -fPIC -shared -o lib$(soname).so $(files)

clean:
	rm lib$(soname).so
