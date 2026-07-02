#include <iostream>
#include <string>
#include <vector>
#include <chrono>
#include <algorithm>
#include <limits>
#include <map>
#include <fstream>
#include <sstream>
#include <mpi.h>
#include <glm/glm.hpp>
#include <libDomain/CSVLoader.h>


using clk = std::chrono::high_resolution_clock; // reloj que uso para medir los tiempos

static double calcularDistancia(const glm::dvec2& coord1, const glm::dvec2& coord2) {
    //Función que devuelve la distancia geográfica entre dos puntos usando la fórmula de Haversine
    const double R = 6371.0; // Radio medio terrestre en km
    double lat1 = glm::radians(coord1.x);
    double lon1 = glm::radians(coord1.y);
    double lat2 = glm::radians(coord2.x);
    double lon2 = glm::radians(coord2.y);
    double dlat = lat2 - lat1;
    double dlon = lon2 - lon1;
    double a = std::sin(dlat/2) * std::sin(dlat/2) +
               std::cos(lat1) * std::cos(lat2) *
               std::sin(dlon/2) * std::sin(dlon/2);
    double c = 2 * std::atan2(std::sqrt(a), std::sqrt(1-a));
    return R * c;
}


struct CeldaTipoExtendida { // representa una fila relevante del CSV con id, coordenadas y continente.
    unsigned int id;
    glm::dvec2 coordenadas;
    std::string continente;
};

struct PokemonRaro { //para almacenar el pokemon más raro de cada continente y sus datos
    unsigned int id;
    std::string continente;
    glm::dvec2 coordenadas;
    unsigned long contador;
};

static std::vector<CeldaTipoExtendida> cargarCSVConContinente(const std::string& ruta) {
    //Función para cargar el CSV completo y filtrar solo los continentes válidos.
    std::vector<CeldaTipoExtendida> resultado;
    std::ifstream archivo(ruta);
    if (!archivo.is_open()) {
        throw std::runtime_error("No se pudo abrir el archivo CSV");
    }
    std::string linea;
    std::getline(archivo, linea); // Leo y descarto la cabecera.

    // Recorro todas las líneas del archivo.
    while (std::getline(archivo, linea)) {
        if (linea.empty()) continue;
        std::stringstream ss(linea);
        std::string token;
        std::vector<std::string> tokens;

        // Extraigo cada campo separado por comas.
        while (std::getline(ss, token, ',')) {
            tokens.push_back(token);
        }
        if (tokens.size() < 23) continue; // Valido número de columnas.

        try {
            // Construyo la celda solo con los datos requeridos.
            CeldaTipoExtendida celda;
            celda.id = std::stoul(tokens[0]);
            celda.coordenadas.x = std::stod(tokens[1]);
            celda.coordenadas.y = std::stod(tokens[2]);

            // Obtengo y limpio el continente.
            std::string cont = tokens[22];
            auto pos = cont.find('/');
            if (pos != std::string::npos) cont = cont.substr(0, pos);
            auto trim = [](std::string &s) {
                s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) { return !std::isspace(ch); }));
                s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) { return !std::isspace(ch); }).base(), s.end());
            };
            trim(cont);
            // Solo acepto los continentes válidos.
            if (cont != "America" && cont != "Europe" && cont != "Africa" &&
                cont != "Asia" && cont != "Pacific") continue;
            celda.continente = cont;
            resultado.push_back(celda);
        } catch (...) { continue; }
    }
    archivo.close();
    return resultado;
}

static void imprimirEstadisticasHistograma(const std::vector<unsigned long>& histograma) {
    //Imprime estadísticas generales sobre el histograma de avistamientos
    unsigned long total = 0;
    unsigned int idRaro = 0, idFrecuente = 0;
    unsigned long minimo = ULONG_MAX, maximo = 0;
    for (unsigned int id = 1; id < histograma.size(); ++id) {
        unsigned long c = histograma[id];
        total += c;
        if (c < minimo) { minimo = c; idRaro = id; }
        if (c > maximo) { maximo = c; idFrecuente = id; }
    }
    std::cout << "Total avistamientos (suma histograma): " << total << "\n";
    std::cout << "ID más raro (MPI): " << idRaro << " con " << minimo << " avistamientos\n";
    std::cout << "ID más frecuente (MPI): " << idFrecuente << " con " << maximo << " avistamientos\n";
}


static std::vector<int> resolverTSPGreedy(const std::vector<PokemonRaro>& pokemons) {
    //Algoritmo greedy para calcular la ruta TSP entre los pokemons raros.
    int n = pokemons.size();
    std::vector<bool> visitado(n, false);
    std::vector<int> ruta;
    int actual = 0;
    ruta.push_back(actual);
    visitado[actual] = true;
    for (int paso = 1; paso < n; ++paso) {
        double minimaDistancia = std::numeric_limits<double>::max();
        int siguiente = -1;
        for (int i = 0; i < n; ++i) {
            if (!visitado[i]) {
                double distancia = calcularDistancia(
                    pokemons[actual].coordenadas,
                    pokemons[i].coordenadas
                );
                if (distancia < minimaDistancia) {
                    minimaDistancia = distancia;
                    siguiente = i;
                }
            }
        }
        if (siguiente != -1) {
            ruta.push_back(siguiente);
            visitado[siguiente] = true;
            actual = siguiente;
        }
    }
    return ruta;
}


int main(int argc, char** argv){
    // Inicio la ejecución MPI y obtengo rango y cantidad de procesos activos
    MPI_Init(&argc, &argv);
    int rango = 0, cantidad = 1;
    MPI_Comm_rank(MPI_COMM_WORLD, &rango);
    MPI_Comm_size(MPI_COMM_WORLD, &cantidad);

    // Compruebo y proceso los argumentos de entrada.
    if(argc < 2){
        if(rango == 0)
            std::cerr << "Uso: mpirun -np 5 computeDistributed <ruta_csv> [--id N]\n";
        MPI_Finalize();
        return 1;
    }



    // Ruta del CSV y parámetro opcional de id objetivo.
    std::string rutaCSV = argv[1];
    unsigned int idObjetivo = 25;
    for(int i = 2; i < argc; ++i){
        std::string a = argv[i];
        if(a == "--id" && i + 1 < argc)
            idObjetivo = static_cast<unsigned int>(std::stoul(argv[++i]));
    }

    // Mapas para repartir los continentes entre los procesos MPI
    std::map<std::string, int> continenteARango = {
        {"America", 0},
        {"Europe", 1},
        {"Africa", 2},
        {"Asia", 3},
        {"Pacific", 4}
    };
    std::vector<std::string> rangoAContinente = {"America", "Europe", "Africa", "Asia", "Pacific"};



    // el root carga el CSV y filtra los datos válidos
    std::vector<CeldaTipoExtendida> filas;
    if(rango == 0){
        std::cout << "\nPRACTICA 1B VICENTE NAVAS MARTINEZ\n\n"; //Solo informacion
        try{
            filas = cargarCSVConContinente(rutaCSV);
            //Imprimo los datos leidos para informar al usuario de que todo funciona correctamente
            std::cout << "------INFORMACION------\n";
            std::cout << "Filas cargadas: " << filas.size() << " | Procesos: " << cantidad << "\n";
            std::cout << "Objetivo ID para Ejercicio 2: " << idObjetivo << "\n";
        } catch(const std::exception& e){
            std::cerr << "Error leyendo CSV: " << e.what() << "\n";
            MPI_Abort(MPI_COMM_WORLD, 2);
        }
    }

    // Difundo el número total de filas entre todos los procesos.
    int N = 0;
    if(rango == 0) N = static_cast<int>(filas.size());
    MPI_Bcast(&N, 1, MPI_INT, 0, MPI_COMM_WORLD);

    // Busco el ID máximo de Pokémon para dimensionar los vectores de contadores.
    unsigned int maximoIdLocal = 0;
    if (rango == 0) {
        for (const auto& c : filas) {
            if (c.id > maximoIdLocal) maximoIdLocal = c.id;
        }
    }
    MPI_Bcast(&maximoIdLocal, 1, MPI_UNSIGNED, 0, MPI_COMM_WORLD);
    unsigned int maximoId = maximoIdLocal;

    // Calculo los tamaños y desplazamientos para repartir las filas proporcionalmente.
    int base = N / cantidad;
    int resto  = N % cantidad;
    int miN  = base + (rango < resto ? 1 : 0);
    std::vector<unsigned int> idsGlobales, idsLocales(miN);
    std::vector<double> latsGlobales, lonsGlobales, latsLocales(miN), lonsLocales(miN);
    std::vector<int> conteos(cantidad), desplazamientos(cantidad);

    // Prepara buffers en root para repartir los datos con Scatterv
    if(rango == 0){
        int offset = 0;
        for(int r = 0; r < cantidad; ++r){
            int cantidadProceso = base + (r < resto ? 1 : 0);
            conteos[r] = cantidadProceso;
            desplazamientos[r] = offset;
            offset += cantidadProceso;
        }
        idsGlobales.resize(N);
        latsGlobales.resize(N);
        lonsGlobales.resize(N);
        for(int i = 0; i < N; ++i){
            idsGlobales[i]  = filas[i].id;
            latsGlobales[i] = filas[i].coordenadas.x;
            lonsGlobales[i] = filas[i].coordenadas.y;
        }
    }

    // Inicio la medición de tiempo del Ejercicio 2
    auto t0 = clk::now();

    // Reparto por Scatterv los datos básicos entre todos los procesos
    MPI_Scatterv(rango == 0 ? idsGlobales.data()  : nullptr, rango == 0 ? conteos.data() : nullptr,
                 rango == 0 ? desplazamientos.data() : nullptr, MPI_UNSIGNED,
                 idsLocales.data(), miN, MPI_UNSIGNED, 0, MPI_COMM_WORLD);
    MPI_Scatterv(rango == 0 ? latsGlobales.data() : nullptr, rango == 0 ? conteos.data() : nullptr,
                 rango == 0 ? desplazamientos.data() : nullptr, MPI_DOUBLE,
                 latsLocales.data(), miN, MPI_DOUBLE, 0, MPI_COMM_WORLD);
    MPI_Scatterv(rango == 0 ? lonsGlobales.data() : nullptr, rango == 0 ? conteos.data() : nullptr,
                 rango == 0 ? desplazamientos.data() : nullptr, MPI_DOUBLE,
                 lonsLocales.data(), miN, MPI_DOUBLE, 0, MPI_COMM_WORLD);

    // Cada proceso cuenta cuántos Pokémon con el ID objetivo ha recibido
    unsigned long contadorLocalId = 0;
    for (int i = 0; i < miN; ++i) {
        if (idsLocales[i] == idObjetivo) ++contadorLocalId;
    }
    // Construyo histograma local de frecuencias por id
    std::vector<unsigned long> histogramaLocal(maximoId + 1, 0);
    for (int i = 0; i < miN; ++i) {
        unsigned int id = idsLocales[i];
        if (id <= maximoId) ++histogramaLocal[id];
    }

    // Sumo los contadores locales con Allreduce, y los histogramas con Reduce
    unsigned long contadorGlobalId = 0;
    MPI_Allreduce(&contadorLocalId, &contadorGlobalId, 1, MPI_UNSIGNED_LONG, MPI_SUM, MPI_COMM_WORLD);
    std::vector<unsigned long> histogramaGlobal(maximoId + 1, 0);
    MPI_Reduce(histogramaLocal.data(), histogramaGlobal.data(), static_cast<int>(histogramaGlobal.size()),
               MPI_UNSIGNED_LONG, MPI_SUM, 0, MPI_COMM_WORLD);

    // Calculo el tiempo total del ejercicio 2
    auto t1 = clk::now();
    double msEj2 = std::chrono::duration<double, std::milli>(t1 - t0).count();

    // El proceso root imprime los resultados y verifica los cálculos
    if (rango == 0) {
        std::cout << "\n------EJERCICIO 2------\n";
        std::cout << "Total ID=" << idObjetivo << " (MPI): " << contadorGlobalId << "\n";
        std::cout << "Tiempo total (ms): " << msEj2 << "\n";
        imprimirEstadisticasHistograma(histogramaGlobal);
        unsigned long totalHistograma = 0;
        for (unsigned long v : histogramaGlobal) totalHistograma += v;
        std::cout << "Verificación: N = " << N << " | suma(histograma) = " << totalHistograma
                  << (totalHistograma == static_cast<unsigned long>(N) ? " ✓\n" : " ✗\n");
    }

    // Compruebo que hay al menos 5 procesos antes de pasar al ejercicio 3
    if (cantidad < 5) {
        if (rango == 0) {
            std::cerr << "\nEl ejercicio 3 necesita 5 procesos, no hay los suficientes\n";
        }
        MPI_Finalize();
        return 0;
    }

    // Inicio la medición de tiempo del Ejercicio 3
    auto t2 = clk::now();

    // Variable donde guardo los datos del continente correspondiente a cada rango
    std::vector<CeldaTipoExtendida> datosContinenteLocal;

    // El root reparte las filas agrupadas por continente entre los cinco procesos
    if (rango == 0) {
        std::map<std::string, std::vector<CeldaTipoExtendida>> mapaContinente;
        for (const auto& fila : filas) {
            std::string cont = fila.continente;
            if (continenteARango.count(cont)) {
                mapaContinente[cont].push_back(fila);
            }
        }

        // Para cada rango, envío los datos de su continente correspondiente
        for (int r = 0; r < 5; ++r) {
            std::string cont = rangoAContinente[r];
            const auto& datos = mapaContinente[cont];
            int cantidadEnviar = static_cast<int>(datos.size());
            if (r == 0) {
                datosContinenteLocal = datos;
            } else {
                MPI_Send(&cantidadEnviar, 1, MPI_INT, r, 0, MPI_COMM_WORLD);
                std::vector<unsigned int> idsEnviar(cantidadEnviar);
                std::vector<double> latsEnviar(cantidadEnviar), lonsEnviar(cantidadEnviar);
                for (int i = 0; i < cantidadEnviar; ++i) {
                    idsEnviar[i] = datos[i].id;
                    latsEnviar[i] = datos[i].coordenadas.x;
                    lonsEnviar[i] = datos[i].coordenadas.y;
                }
                MPI_Send(idsEnviar.data(), cantidadEnviar, MPI_UNSIGNED, r, 1, MPI_COMM_WORLD);
                MPI_Send(latsEnviar.data(), cantidadEnviar, MPI_DOUBLE, r, 2, MPI_COMM_WORLD);
                MPI_Send(lonsEnviar.data(), cantidadEnviar, MPI_DOUBLE, r, 3, MPI_COMM_WORLD);
            }
        }
    // Todos los rangos excepto el root reciben sus datos por continente
    } else if (rango < 5) {
        int cantidadRecibir = 0;
        MPI_Recv(&cantidadRecibir, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        std::vector<unsigned int> idsRecibir(cantidadRecibir);
        std::vector<double> latsRecibir(cantidadRecibir), lonsRecibir(cantidadRecibir);
        MPI_Recv(idsRecibir.data(), cantidadRecibir, MPI_UNSIGNED, 0, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        MPI_Recv(latsRecibir.data(), cantidadRecibir, MPI_DOUBLE, 0, 2, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        MPI_Recv(lonsRecibir.data(), cantidadRecibir, MPI_DOUBLE, 0, 3, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        datosContinenteLocal.resize(cantidadRecibir);
        for (int i = 0; i < cantidadRecibir; ++i) {
            datosContinenteLocal[i].id = idsRecibir[i];
            datosContinenteLocal[i].coordenadas = glm::dvec2(latsRecibir[i], lonsRecibir[i]);
        }
    }

    // Cada proceso calcula el Pokémon menos frecuente de su continente
    PokemonRaro pokemonRaroLocal;
    pokemonRaroLocal.continente = (rango < 5) ? rangoAContinente[rango] : "";
    pokemonRaroLocal.contador = ULONG_MAX;
    pokemonRaroLocal.id = 0;
    if (rango < 5 && !datosContinenteLocal.empty()) {
        std::map<unsigned int, std::vector<glm::dvec2>> mapaLocal;
        for (const auto& celda : datosContinenteLocal) {
            mapaLocal[celda.id].push_back(celda.coordenadas);
        }
        for (const auto& par : mapaLocal) {
            unsigned long cnt = par.second.size();
            if (cnt < pokemonRaroLocal.contador) {
                pokemonRaroLocal.contador = cnt;
                pokemonRaroLocal.id = par.first;
                pokemonRaroLocal.coordenadas = par.second[0];
            }
        }
    }


    // Preparo buffers y recopilo los cinco pokémons raros en el root con MPI_Gather
    std::vector<PokemonRaro> todosPokemonsRaros(5);
    unsigned long totalAvistamientosContinente = datosContinenteLocal.size();
    double bufferEnviar[5] = {
        static_cast<double>(pokemonRaroLocal.id),
        pokemonRaroLocal.coordenadas.x,
        pokemonRaroLocal.coordenadas.y,
        static_cast<double>(pokemonRaroLocal.contador),
        static_cast<double>(totalAvistamientosContinente)
    };

    std::vector<double> bufferRecibir;
    if (rango == 0) bufferRecibir.resize(5 * 5);
    MPI_Gather(bufferEnviar, 5, MPI_DOUBLE,
               rango == 0 ? bufferRecibir.data() : nullptr, 5, MPI_DOUBLE,
               0, MPI_COMM_WORLD);


    // En el root reconstruyo el vector de Pokémon raros y calculo la ruta TSP
    if (rango == 0) {
        for (int i = 0; i < 5; ++i) {
            todosPokemonsRaros[i].id = static_cast<unsigned int>(bufferRecibir[i*5]);
            todosPokemonsRaros[i].coordenadas = glm::dvec2(bufferRecibir[i*5 + 1], bufferRecibir[i*5 + 2]);
            todosPokemonsRaros[i].contador = static_cast<unsigned long>(bufferRecibir[i*5 + 3]);
            todosPokemonsRaros[i].continente = rangoAContinente[i];
        }
        std::cout << "\n------EJERCICIO 3------\n";
        for (int i = 0; i < 5; ++i) {
            const auto& rp = todosPokemonsRaros[i];
            unsigned long totalCont = static_cast<unsigned long>(bufferRecibir[i*5 + 4]);
            std::cout << rp.continente << ": ID=" << rp.id
                      << " | Apariciones=" << rp.contador << " de " << totalCont
                      << " | Coordenadas=(" << rp.coordenadas.x << ", " << rp.coordenadas.y << ")\n";
        }
        std::vector<int> ruta = resolverTSPGreedy(todosPokemonsRaros);
        std::cout << "\n------Ruta Óptima segun Greedy TSP------\n";
        double distanciaTotal = 0.0;
        std::cout << "Inicio: " << todosPokemonsRaros[ruta[0]].continente
                  << " (ID " << todosPokemonsRaros[ruta[0]].id << ")\n";
        for (size_t i = 1; i < ruta.size(); ++i) {
            double distancia = calcularDistancia(
                todosPokemonsRaros[ruta[i-1]].coordenadas,
                todosPokemonsRaros[ruta[i]].coordenadas
            );
            distanciaTotal += distancia;
            std::cout << "  → Viaje a " << todosPokemonsRaros[ruta[i]].continente
                      << " (ID " << todosPokemonsRaros[ruta[i]].id << ")"
                      << " | Distancia: " << distancia << " km"
                      << " | Acumulada: " << distanciaTotal << " km\n";
        }
        auto t3 = clk::now();
        double msEj3 = std::chrono::duration<double, std::milli>(t3 - t2).count();
        std::cout << "\nDistancia total recorrida: " << distanciaTotal << " km\n";
        std::cout << "Tiempo Ejercicio 3 (ms): " << msEj3 << "\n";
    }


    // Finalizo la ejecución paralela y libero los recursos MPI
    MPI_Finalize();
    return 0;
}
