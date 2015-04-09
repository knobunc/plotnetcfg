CFLAGS=-W -Wall

plotnetcfg: args.o dot.o ethtool.o handler.o if.o label.o main.o match.o netlink.o \
	    netns.o tunnel.o utils.o \
	    parson/parson.o \
	    handlers/bridge.o handlers/master.o handlers/openvswitch.o handlers/veth.o \
	    handlers/vlan.o
	gcc -o $@ $+

args.o: args.c args.h
dot.o: dot.c dot.h handler.h if.h label.h netns.h utils.h version.h
ethtool.o: ethtool.c ethtool.h
handler.o: handler.c handler.h if.h netns.h
if.o: if.c if.h compat.h ethtool.h handler.h label.h netlink.h utils.h
label.o: label.h label.c utils.h
main.o: main.c args.h dot.h handler.h netns.h utils.h version.h
match.o: match.c match.h if.h netns.h
netlink.o: netlink.c netlink.h utils.h
netns.o: netns.c netns.h compat.h handler.h if.h match.h utils.h
tunnel.o: tunnel.c tunnel.h handler.h if.h match.h netns.h utils.h tunnel.h
utils.o: utils.c utils.h if.h netns.h

parson/parson.c: parson/parson.h

handlers/bridge.o: handlers/bridge.c handlers/bridge.h handler.h
handlers/master.o: handlers/master.c handlers/master.h handler.h match.h utils.h
handlers/openvswitch.o: handlers/openvswitch.h parson/parson.h args.h handler.h label.h match.h tunnel.h utils.h
handlers/veth.o: handlers/veth.c handlers/veth.h handler.h match.h utils.h
handlers/vlan.o: handlers/vlan.c handlers/vlan.h handler.h netlink.h

version.h:
	echo "#define VERSION \"`git describe 2> /dev/null || cat version`\"" > version.h

clean:
	rm -f version.h *.o handlers/*.o plotnetcfg
