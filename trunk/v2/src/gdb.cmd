set pagination off
b main
commands
info locals
continue
end

b buffer_pool.c:35
commands
	#p this->head	
	continue
end

b tst.c:165
commands
	c	
end
start

