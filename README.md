franco@Franco: /mnt/c/Users/franc/Proyectos/ISO8583$
code .


sudo service postgresql start
sudo -u postgres psql
\c transaction_db

TRUNCATE TABLE transactions RESTART IDENTITY;

gcc Server.c -o Server -lpq -I/usr/include/postgresql

gcc Client.c -o Client