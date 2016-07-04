FROM ubuntu:14.04
MAINTAINER Massimo GENGARELLI <massimo.gengarelli@gmail.com>
EXPOSE 8081

# Packages installation
RUN set -x && \
    apt-get update && \ 
    apt-get install -y git build-essential clang libsqlite3-dev

# Clone, build and deploy
RUN set -x && \
    git clone http://github.com/massix/tdj-crawler.git /tmp/tdj-crawler && \
    cd /tmp/tdj-crawler && \
    rm -fr liblali && \
    git clone http://github.com/massix/liblali.git liblali && \
    cd liblali && \
    git fetch --all && \
    git checkout develop && \
    cd /tmp/tdj-crawler && \
    sed -i 's/\.\/conf\/tdj-crawler\.conf/\/etc\/tdj-crawler.conf/' bgg-client/main.cpp && \
    make && \
    make deploy

# Install
RUN set -x && \
    cd /tmp/tdj-crawler && \
    mkdir -p /data && \
    cp bin/liblali.so /usr/lib/ && \
    cp bin/libflate.so /usr/lib/ && \
    cp bin/libjson11.so /usr/lib/ && \
    cp bin/tdj-crawler /usr/bin/ && \
    cp bin/conf/tdj-crawler.conf /etc/ && \
    cp bin/conf/tdj-crawler.users.conf /etc/ && \
    cp -r bin/templates/ /data/ && \
    cp -r bin/resources/ /data/

# Prepare a working out-of-the-box config
RUN set -x && \
    sed -i 's/\.\/conf/\/etc/' /etc/tdj-crawler.conf && \
    sed -i 's/20/43200/' /etc/tdj-crawler.conf && \
    sed -i 's/\.\/trolls.db/\/data\/trolls.db/' /etc/tdj-crawler.conf && \
    sed -i 's/\.\/templates\//\/data\/templates\//g' /etc/tdj-crawler.conf && \
    sed -i 's/\.\/resources\//\/data\/resources\//g' /etc/tdj-crawler.conf && \
    sed -i 's/\.\/static\//\/data\/static\//g' /etc/tdj-crawler.conf

CMD ["/usr/bin/tdj-crawler"]
