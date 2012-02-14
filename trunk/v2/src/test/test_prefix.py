import os
result = os.popen("../test_tst").read()
check=open("prefix.log").read()
print check
assert(result==check)

