echo `for i in *.test; do ./$i; done | grep passed | wc -l` success
echo `/bin/ls -l *.test | wc -l` in all