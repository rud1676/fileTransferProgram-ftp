# linux - ftp 서버 구현

시스템 프로그래밍에서 과제로 구현한 ftp 클라이언트/서버 프로그램. child, parent프로세스와 프로세스간에 시그널을 통하여 multiuser환경을 지원. ubuntu 16.04 환경에서 진행하였고, 파일을 주고받는 data-connection 소캣과 응답메세지 즉, 행동처리에 대한 메세지를 message-connection을 통하여 split connection방식으로 구현.

## How-to-Use

terminal 1

```
$ make
$ ./srv 10000
```

terminal 2

```
$ ./cli 127.0.0.1 10000
$ username: test1
$ password: 12
```

client 명령어 list

- ls: 기존 ls명령과 같습니다.
- dir: 폴더 리스트를 보여줍니다.
- pwd: 현재 접속한 서버의 경로를 보여줍니다.
- cd (path): 경로를 변경합니다
- mkdir (name): 현재 경로에 폴더를 생성합니다.
- delete (name): 파일을 삭제합니다.
- rmdir: directory를 삭제합니다.
- rename: 파일 이름을 수정합니다.
- quit: client와 해당 서버의 process를 종료합니다.
- bin: 파일 전송 모드를 binary모드로 바꿉니다.
- ascii: 파일 전송 모드를 ascii모드로 바꿉니다.
- get (name): 서버에 존재하는 파일을 클라이언트로 받아옵니다.
- put (name): 클라이언트에 존재하는 파일을 서버로 전송합니다.

## 결과화면

[결과1](./img/캡처.PNG)
[결과2](./img/2.PNG)

## 코드설명

코드 내부에 주석을 달았습니다.
