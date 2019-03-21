# first time create Readme.md

# a Concurrency and throughput test experiment process with boost_1_69, can use console to control. 
# see 
###
 #  simple connection to server:
 #   - logs in just with username (no password)
 #   - all connections are initiated by the client: client asks, server answers
 #   - server disconnects any client that hasn't pinged for XXX seconds (timeout )
 #
 #   Possible response:
 #   - login success or not
 #   - gets a list of all connected clients
 #   - ping: the server answers either with "ping ok from srv" or "ping client_list_changed"
 #
