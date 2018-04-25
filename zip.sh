ZIP="proj10.zip"

make clean
mv bin/sor sor
zip -r $ZIP lib/* src/* Makefile bin/
mv sor bin/sor

echo "Send $ZIP to joao.sobrinho@lx.it.pt";
