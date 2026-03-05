FROM itsafeaturemythic/mythic_python_base:latest

RUN apt update
RUN apt install -y gcc-mingw-w64-i686
RUN apt install -y gcc-mingw-w64-x86-64
RUN apt install -y default-jre
RUN apt install -y zip

RUN mkdir /crystal-palace/
ADD https://tradecraftgarden.org/download/cpdist20260303.tgz /crystal-palace.tgz
RUN tar -xvzf /crystal-palace.tgz -C /crystal-palace/

WORKDIR /Mythic/
CMD ["python3", "main.py"]
