find . -iname .moc -exec rm -rf {} \;
find . -iname .obj -exec rm -rf {} \;
find . -iname release -exec rm -rf {} \;
find . -iname debug -exec rm -rf {} \;
find . -iname .qmake.stash -exec rm -rf {} \; 
find . -iname *.qm -exec rm -f {} \;
rm -f global_variables.pri
rm -f log.txt
rm -f tupitube.desk.pro.user.*
rm -f tupitube.desk.pro.user
rm -rf .git
rm -rf .github
