Tiny 웹 서버  
Dave O'Hallaron  
Carnegie Mellon University

이곳은 Tiny 서버의 홈 디렉터리입니다. Tiny 서버는 Carnegie Mellon University의 "15-213: Intro to Computer Systems" 과목에서 사용하는 200줄 규모의 웹 서버입니다. Tiny는 GET 메서드를 사용해 ./ 디렉터리에서 정적 콘텐츠(텍스트, HTML, GIF, JPG 파일)를 제공하고, ./cgi-bin 디렉터리의 CGI 프로그램을 실행해 동적 콘텐츠를 제공합니다. 기본 페이지는 home.html이며, 이는 디렉터리 내용을 브라우저에서 확인할 수 있도록 index.html 대신 사용합니다.

Tiny는 안전하지도 완전하지도 않지만, 실제 웹 서버가 어떻게 동작하는지 이해하는 데 도움을 줍니다. 교육 목적에 한해 사용하세요.

이 코드는 Linux 2.2.20 커널에서 gcc 2.95.3으로 깨끗하게 컴파일되고 실행됩니다.

Tiny 설치 방법:  
   깨끗한 디렉터리에서 "tar xvf tiny.tar"를 실행하세요.

Tiny 실행 방법:  
   서버 머신에서 "tiny <port>"를 실행하세요.  
        예: "tiny 8000"  
   브라우저에서 Tiny에 접속하세요:  
        정적 콘텐츠: http://<host>:8000  
        동적 콘텐츠: http://<host>:8000/cgi-bin/adder?1&2

파일 목록:  
  tiny.tar            이 디렉터리 전체를 담은 아카이브  
  tiny.c              Tiny 서버 소스 코드  
  Makefile            tiny.c를 위한 메이크파일  
  home.html           테스트용 HTML 페이지  
  godzilla.gif        home.html에 포함된 이미지  
  README              이 파일  
  cgi-bin/adder.c     두 수를 더하는 CGI 프로그램  
  cgi-bin/Makefile    adder.c를 위한 메이크파일
