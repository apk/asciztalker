asciztalker
===========

A wrapper to talk a forwarding protocol into a transport program
(like netcat, openssl s_client)... The protocol is to send a
null-terminated ascii string (hence 'asciz'), and expect a null-terminated
response string which is empty on success or else contains an error
message.
