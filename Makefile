triagem: main.cpp Paciente.cpp Atendimento.cpp Paciente.h Atendimento.h Historico.h TabelaHash.h ArvoreAVL.h Gravidade.h
	g++ -Wall -Wextra -std=c++11 -o triagem main.cpp Paciente.cpp Atendimento.cpp

# o benchmark tem main() proprio, entao compila separado do sistema.
# o -O2 e importante: sem ele os tempos medidos nao representam o programa real.
benchmark: benchmark.cpp TabelaHash.h
	g++ -Wall -Wextra -std=c++11 -O2 -o benchmark benchmark.cpp

clean:
	rm -f triagem triagem.exe benchmark benchmark.exe *.o
