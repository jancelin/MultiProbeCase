# POSTGRESQL
```
sudo apt install postgresql-14 postgresql-14-postgis-3 postgresql-14-cron postgresql-14-postgis-3-scripts postgresql-plpython3-14
echo "deb https://packagecloud.io/timescale/timescaledb/debian/ $(lsb_release -c -s) main" | sudo tee /etc/apt/sources.list.d/timescaledb.list
wget --quiet -O - https://packagecloud.io/timescale/timescaledb/gpgkey | sudo apt-key add -
sudo apt-get update
sudo apt install timescaledb-2-postgresql-14
```
* modify postgresql.conf
```
sudo nano /etc/postgresql/14/main/postgresql.conf
```
* add
```
listen_addresses = '*'
shared_preload_libraries = 'timescaledb,pg_cron'
cron.database_name = 'mpc'
```
* modify pg_hba.conf
  * add
```
host    all             all             0.0.0.0/0               md5
```
  * restart
```
systemctl restart postgresql
```
* change password
```
sudo -u postgres psql
```
* connect to postgres
```
psql -U postgres -h localhost
```
* create db
```
CREATE database mpc;
\c mpc
CREATE EXTENSION IF NOT EXISTS timescaledb;
CREATE EXTENSION IF NOT EXISTS postgis;
CREATE EXTENSION IF NOT EXISTS pg_cron cascade;
```
* Verify

```\dx```

```
                                                List of installed extensions
    Name     | Version |   Schema   |                                      Description                                      
-------------+---------+------------+---------------------------------------------------------------------------------------
 pg_cron     | 1.5     | pg_catalog | Job scheduler for PostgreSQL
 plpgsql     | 1.0     | pg_catalog | PL/pgSQL procedural language
 postgis     | 3.3.3   | public     | PostGIS geometry and geography spatial types and functions
 timescaledb | 2.11.0  | public     | Enables scalable inserts and complex queries for time-series data (Community Edition)
(4 rows)
```
* create table

```
CREATE SCHEMA cyclopee;
---- ##Creation table stockage des données en json: https://scalegrid.io/blog/using-jsonb-in-postgresql-how-to-effectively-store-index-json-data-in-postgresql/
--DROP TABLE cyclopee.sensor;
CREATE TABLE cyclopee.sensor
(
	time TIMESTAMPTZ NOT NULL, -- time: heure d'arrivé du message dans le json (extract with node-red)
        data jsonb NOT NULL, -- data des sensors au format json sans le time
	sync integer -- synchro data
);
-- ## Installation de l'extention timescale_db https://docs.timescale.com/self-hosted/latest/install/installation-linux/
-- ## Creation hypertable
-- ## https://docs.timescale.com/api/latest/hypertable/create_hypertable/
SELECT create_hypertable('cyclopee.sensor','time', chunk_time_interval => INTERVAL '1 hour');
-- ## Création de l'index
-- ## https://docs.timescale.com/use-timescale/latest/schema-management/json/#index-individual-fields
CREATE INDEX idx_sensor ON cyclopee.sensor (((data->>'id')::text), time DESC);
```
* exit

```\q```

## GRAFANA

```
sudo apt-get install -y apt-transport-https
sudo apt-get install -y software-properties-common wget
sudo wget -q -O /usr/share/keyrings/grafana.key https://apt.grafana.com/gpg.key
echo "deb [signed-by=/usr/share/keyrings/grafana.key] https://apt.grafana.com stable main" | sudo tee -a /etc/apt/sources.list.d/grafana.list
sudo apt-get update
sudo apt-get install grafana
sudo /bin/systemctl daemon-reload
sudo /bin/systemctl enable grafana-server
sudo /bin/systemctl start grafana-server
```

## RFCOMM

```
sudo rfcomm connect 0 98:D3:B1:FD:C3:2C
```

## RTK

```
sudo apt-get install -y build-essential
git clone https://github.com/rtklibexplorer/RTKLIB.git
cd RTKLIB/
sudo make --directory=RTKLIB/app/consapp/str2str/gcc
sudo make --directory=RTKLIB/app/consapp/str2str/gcc install
```

* RUN

```
str2str -in ntrip://:@caster.centipede.fr:80/LIENSS -out serial://rfcomm0:115200:8:N:1:off
```
