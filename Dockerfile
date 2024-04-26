FROM ubuntu:22.04

WORKDIR /workdir

COPY dependencies.txt .
RUN apt-get update && apt-get -y install $(cat dependencies.txt)

COPY . .

RUN mkdir -p _build
RUN cd _build && \
  cmake -DUSEWX=yes -DCMAKE_BUILD_TYPE=Release .. && \
  cmake --build . -j$(nproc --all)

CMD /workdir/_build/install/far2l
