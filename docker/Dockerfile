FROM dockcross/linux-armv6

ENV DEFAULT_DOCKCROSS_IMAGE my_cool_image
RUN bash -c "echo \"deb http://ftp.de.debian.org/debian sid main\" >> /etc/apt/sources.list"
RUN apt-get update --yes
RUN apt-get install --no-install-recommends --yes \
    cpp-7-arm-linux-gnueabihf \
    g++-7-arm-linux-gnueabihf
RUN apt-get clean --yes

# Set version
ENV CROSS_TRIPLE arm-linux-gnueabihf
ENV CROSS_TRIPLE_VERSION 7
ENV CROSS_ROOT /usr/${CROSS_TRIPLE}
ENV AS=/usr/bin/${CROSS_TRIPLE}-as-${CROSS_TRIPLE_VERSION} \
    AR=/usr/bin/${CROSS_TRIPLE}-ar-${CROSS_TRIPLE_VERSION} \
    CC=/usr/bin/${CROSS_TRIPLE}-gcc-${CROSS_TRIPLE_VERSION} \
    CPP=/usr/bin/${CROSS_TRIPLE}-cpp-${CROSS_TRIPLE_VERSION} \
    CXX=/usr/bin/${CROSS_TRIPLE}-g++-${CROSS_TRIPLE_VERSION} \
    LD=/usr/bin/${CROSS_TRIPLE}-ld-${CROSS_TRIPLE_VERSION} \
    FC=/usr/bin/${CROSS_TRIPLE}-gfortran-${CROSS_TRIPLE_VERSION}