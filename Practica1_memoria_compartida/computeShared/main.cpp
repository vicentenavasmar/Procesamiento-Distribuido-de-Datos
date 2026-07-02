#include <iostream>
#include <string>
#include <vector>
#include <chrono>
#include <algorithm>
#include <numeric>
#include <unordered_map>
#include <omp.h>

#include <libDomain/CSVLoader.h>
#include <libDomain/Compute.h>
#include <libDomain/data/Names.h>

using clk = std::chrono::system_clock; //Para poder usar el reloj durante la practica

int main(int argc, char *argv[])
{
    if ( argc < 2 ) //Si no se pasa ningun csv se comunica y cierra
    {
        std::cout << "CSV file not found. Please use the first argument to set the file to be processed..." << std::endl;
        return -1;
    }

    std::string filePath = argv[1]; //Sacamos la direccion del csv que nos han dado

    compute::ExerciseLoader csvLoader( filePath ); //Sacamos el csv gracias csvLoader

    const auto& rows = csvLoader.data();  // vector de CellType {id, coordinates}

    printf("<<Los datos de tiempo seran mostrados en nanosegundos(ns) en lugar de milisegundos(ms) para representarlos con mayor precision>>\n");

    const unsigned int maxThreads = omp_get_num_procs(); //Este es el numero de threads disponibles

    printf("Threads disponibles: %i\n", maxThreads); //imprimimos el numero de threads disponibles

    // Por defecto, usamos la mitad de cores como pide el apartado 3
    //unsigned int numThreads { std::max(1u, maxThreads / 2) };
    unsigned int numThreads = maxThreads;              // por defecto, el número de hilos es el maximo

    if( argc == 3)
    {
        numThreads = std::max(1, std::stoi( argv[2]));//Si se recibe un argumento mas es el numero de threads elegido por el usuario
    }

    omp_set_num_threads(numThreads);

    //De primeras se imprime informacion de las filas y threads usados para informar al usuario
    std::cout << "Filas: " << rows.size() << "\n";
    std::cout << "Threads usados: " << numThreads << "\n\n";

    // Apartado 1
    {
        const unsigned int pikachuId = 25; //la id de pikachu que buscamos
        const int runs = 10; //Veces que repetimos ejecutamos
        long long sum_seq_ns = 0, sum_par_ns = 0; //Inicializamos el tiempo que tardan a 0 en ambas formas
        unsigned long ref_count = 0;

        for (int r = 0; r < runs; ++r) { //Bucle, cada vuelta es una ejecución
            auto t0 = clk::now(); //Guardamos el momento en el que empieza el bucle monohilo (sequencial)
            unsigned long count_seq = 0; //Contador para monohilo
            for (size_t i = 0; i < rows.size(); ++i) { //Bucle que va recorriendo las filas
                if (rows[i].id == pikachuId) ++count_seq; //Cada vez que encuenta un pikachu aumenta el contador
            }

            auto t1 = clk::now(); //Guardamos el momento en elq eu acaba el bucle monohilo de contar pikachus
            sum_seq_ns += std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count();
            //Aqui se almacena el tiempo que ha tardado

            t0 = clk::now(); //Guardamos el momento en el que empieza la reduccion paralela
            unsigned long count_par = 0; //contador
            #pragma omp parallel for reduction(+:count_par) //Directiva de openMP para la reduccion paralela
            for (long i = 0; i < static_cast<long>(rows.size()); ++i) { //Bucle que va recorriendo als filas
                if (rows[i].id == pikachuId) ++count_par;//Cada evz que encuentra un pikachu aumenta el contador
            }
            t1 = clk::now();//Gaurdamos el momento en el que acaba la reduccion paralela de contar pikachus
            sum_par_ns += std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count();
            //Se alamacena el tiempo que ha tardado en contar todos

            if (r == 0) ref_count = count_seq; //Aqui se almacena el numero de pikachus para imprimirlo despues
        }

        //Aqui imprimimos los resultados
        std::cout << "----- Apartado 1 -----\n";
        std::cout << "Pikachu encontrados: " << (rows.empty() ? 0 : (int)ref_count) << "\n";
        std::cout << "Tiempo medio de mono-hilo (ns): " << (sum_seq_ns / runs) << "\n";
        std::cout << "Tiempo medio de reduccion paralela (ns): " << (sum_par_ns / runs) << "\n";
        //imprimos la conclusion según los tiempos medios de cada forma
        if ((sum_par_ns / runs) < (sum_seq_ns / runs)) {
            std::cout << "Conclusion: Reduccion paralela es mas rapido para estas condiciones\n\n";
        } else if ((sum_par_ns / runs) > (sum_seq_ns / runs)) {
            std::cout << "Conclusion: Mono-hilo es mas rapido para estas condiciones\n\n";
        } else {
            std::cout << "Conclusion: Ambos tienen una velocidad similar\n\n";
        }
    }

    // Apartado 2
    unsigned int maxId = 0; //Inicializamos la variable con el id maximo
    for (const auto& c : rows){//Bucle que recorre las filas buscando el id maximo
        if (c.id > maxId) {
            maxId = c.id;
        }
    }

    if (maxId == 0){
        maxId = 151;//Si no se ecncuentra nada se asigna el maximo(ya lo sabemos)
    }


    {
        const int runs = 10; //numero ejecuciones
        long long sum_seq_ns = 0, sum_par_ns = 0; //Inicializamos las variables para veer cuanto tiempo tardan
        std::vector<unsigned int> freq_seq, freq_par; //Aqui guardamos el vector de la primera vuelta
        unsigned int rare_id_seq = 0, freq_id_seq = 0; //Guardan el mas raro y mas frecuente en seeqencia y paraleo
        unsigned int rare_id_par = 0, freq_id_par = 0;

        for (int r = 0; r < runs; ++r) { //El bucle apra hacer el numero de ejcuciones
            // Secuencial/Monohilo
            auto t0 = clk::now(); //Se guarda el instante inicial
            std::vector<unsigned int> freq(maxId + 1, 0);
            //Se crea el vectr en el que se posiciona cada pokemon y su numero de aparaciones(que empiezan en 0)
            for (size_t i = 0; i < rows.size(); ++i) {
            //se recorren las filas para ir aumeando el numero de apariciones
                if (rows[i].id <= maxId) ++freq[rows[i].id]; //MaxIds es para evitar desbordes
            }
            auto t1 = clk::now(); //Se guarda el momento final
            sum_seq_ns += std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count();
            //se saca el tiempo que se tarda en secuencial
            if (r == 0) { //Si es la primera vuelta, se guarda el vector
                freq_seq = freq;

                unsigned int min_cnt = UINT_MAX, min_id = 1;
                unsigned int max_cnt = 0, max_idv = 1;
                for (unsigned int id = 1; id <= maxId; ++id) {
                    //Recorre el todo el vector para eencontrar el mas raro y el mas frecuente
                    if (freq[id] < min_cnt) { min_cnt = freq[id]; min_id = id; }
                    if (freq[id] > max_cnt) { max_cnt = freq[id]; max_idv = id; }
                }
                rare_id_seq = min_id; //Guardamos el mas raro y mas frecuente en secuencial
                freq_id_seq = max_idv;
            }

            // Paralelo
            t0 = clk::now();//Capturamos el momento en el que empieza la reduccion paralela
            std::vector<unsigned int> freq_global(maxId + 1, 0); //Vectoor compartido
            #pragma omp parallel //Inicia la region paralela
            {
                std::vector<unsigned int> freq_local(maxId + 1, 0);//Cada hilo crea su vector
                #pragma omp for nowait //hilos paralelos sin barreras
                for (long i = 0; i < static_cast<long>(rows.size()); ++i) {  //Cada hilo cuenta sobre su vector local
                    unsigned int id = rows[i].id;
                    if (id <= maxId) ++freq_local[id];
                }
                #pragma omp critical //Cada hilo, al terminar, suma su historial local en el global
                {
                    for (unsigned int id = 1; id <= maxId; ++id) {
                        freq_global[id] += freq_local[id];
                    }
                }
            }
            t1 = clk::now();//Se captura cuanto ha tardado
            sum_par_ns += std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count();
            //Este es el tiempo que ha tardado en paralelo

            if (r == 0) { //Igual que en secuencial
                freq_par = freq_global;
                unsigned int min_cnt = UINT_MAX, min_id = 1;
                unsigned int max_cnt = 0, max_idv = 1;
                for (unsigned int id = 1; id <= maxId; ++id) {
                    if (freq_global[id] < min_cnt) { min_cnt = freq_global[id]; min_id = id; }
                    if (freq_global[id] > max_cnt) { max_cnt = freq_global[id]; max_idv = id; }
                }
                rare_id_par = min_id; //Guardamos el mas raro y mas frecuente en paralelo
                freq_id_par = max_idv;
            }
        }

        //Aqui imprimimos los resultados
        std::cout << "----- Apartado 2 -----\n";
        std::cout << "ID Más Frecuente: " << freq_id_seq << " (seq) | " << freq_id_par << " (par)\n";
        std::cout << "ID Más Raro: " << rare_id_seq << " (seq) | " << rare_id_par << " (par)\n";
        std::cout << "Tiempo Medio De MonoHilo (ns): " << (sum_seq_ns / runs) << "\n";
        std::cout << "Tiempo Medio De Reduccion Paralela (ns): " << (sum_par_ns / runs) << "\n";
        if ((sum_par_ns / runs) < (sum_seq_ns / runs)) {
            std::cout << "Conclusion: Reduccion paralela es mas rapido para estas condiciones\n\n";
        } else if ((sum_par_ns / runs) > (sum_seq_ns / runs)) {
            std::cout << "Conclusion: Secuencial es mas rapido para estas condiciones\n\n";
        } else {
            std::cout << "Conclusion: Ambos tienen una velocidad similar\n\n";
        }
    }

    // Apartado 3
    {
        std::cout << "----- Apartado 3 -----\n";
        omp_set_num_threads(std::max(1u, maxThreads/2)); // asegurar mitad de cores
        const glm::dvec2 target(20.525750, -97.46000); //Este el punto objetivo
        const double radius_km = 30.0; //rango máximo de búsqueda

        auto nearest_in_subset = [&](const std::vector<compute::CellType>& subset) {
            //Funcion lamba que recibe el vector y devuelve el pair ID/distancia del pokemon más cercano
            //Con [&] se pueden usar variables exteriores
            double best_d = 1e300; //inicializamos la variable de distancia del pokemon que buscamos al punto a una distancia muy grande
            unsigned int best_id = 0; //Inicializamso el id del pokemon que buscamos
            #pragma omp parallel //Empiezza region paralela
            {
                double local_best_d = 1e300; //Cada hilo tiene su propio mejor candidato
                unsigned int local_best_id = 0;
                #pragma omp for nowait //Seccion sin barrera
                for (long i = 0; i < static_cast<long>(subset.size()); ++i) { //recorre los candidatos
                    const auto& c = subset[i]; //Saca el candidato
                    double d = compute::calculateDistance(target, c.coordinates); //Calcula su distancia al punto
                    if (d <= radius_km && d < local_best_d) { //Si este candidato esta mas cerca que el que teniamos como mejor, este pasa a ser el mejor
                        local_best_d = d;
                        local_best_id = c.id;
                    }
                }
                #pragma omp critical //Seccion critica cuanddooo termina
                {
                    if (local_best_d < best_d) { //Cada hijo comprueba si su mejor candidato es mejor que el mejor global
                        best_d = local_best_d; //Si lo es, este pasa a ser el nuevo mejor global
                        best_id = local_best_id;
                    }
                }
            }
            return std::pair<unsigned int,double>(best_id, best_d); //Se devuelve el par (id del mejor, su id )
        };

        // 1000 ultimas
        std::vector<compute::CellType> lastK; //Vector donde almacenamos las ultimas 1000
        {
            size_t K = std::min<size_t>(1000, rows.size()); //K es 1000 si el tamaño es mas grande, y si no el tamaño
            lastK.reserve(K); //reserva la capacidad para k elementos
            for (size_t i = rows.size() - K; i < rows.size(); ++i) lastK.push_back(rows[i]);
            //Se recorre desde el principio de los ultimos k numero, y se va metiendo en el nuevo vector
        }

        // 1000 "más frecuentes": por conteo histórico top-K
        std::vector<compute::CellType> topFreqK; //Vector donde almacenamos las 1000 mas frecuentes
        {
            std::vector<unsigned int> freq(maxId + 1, 0); //Vvector con los pokemons, su par (id,frecuencia)
            for (const auto& c : rows) if (c.id <= maxId) ++freq[c.id]; //Recorrre las filas aumentando als frecuencias
            // ordenar ids por frecuencia desc
            std::vector<unsigned int> ids(maxId); //Vector que contendra los pokemons ordenados por frecuencia
            std::iota(ids.begin(), ids.end(), 1u); //Prepara la lista para ordenarla
            std::sort(ids.begin(), ids.end(), [&](unsigned int a, unsigned int b){ return freq[a] > freq[b]; }); //ordena

            size_t K = std::min<size_t>(1000, rows.size()); //K es el minimo entre 1000 y el total del size
            topFreqK.reserve(K); //reserva capacidad para k elementos
            // extraer ocurrencias que pertenezcan al conjunto de ids más frecuentes hasta llenar K
            std::unordered_map<unsigned int,bool> topSet; //Esta tabla hash marca los ids que si pertenecen
            size_t need_ids = std::min<size_t>(ids.size(), 1000); // cuantos IDs frencuentes consideraremos
            for (size_t i = 0; i < need_ids; ++i) topSet[ids[i]] = true; // Marca en el hash los IDs más frecuentes
            for (const auto& c : rows) { // Recorre el antiguo en su orden original para extraer apariciones de IDs “top”
                if (topSet.find(c.id) != topSet.end()) { // Si el ID de esta aparición está en el conjunto de más frecuentes, es candidata
                    topFreqK.push_back(c); // Añade la aparición a topFreqK manteniendo el orden en el CSV
                    if (topFreqK.size() >= K) break; //Corta si ya tenemos las que nos hacen falta
                }
            }
            if (topFreqK.empty()) topFreqK = lastK; // si no se lleno nada usa las ultiams k para no dejarlo vacio
        }

        auto t0 = clk::now(); //guarda el instante en elq eu se empieza a buscar en los 1000 ultimos
        auto [id_last, d_last] = nearest_in_subset(lastK); //busca el mas cercano en los 1000 ultimos
        auto t1 = clk::now(); //Guarda el instante en el que termina de buscar en los 1000 ultimos
        auto ms_last = std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count(); //tiempo que se ha tardado en lso 1000 ultimos

        t0 = clk::now(); //guarda el instante en elq eu se empieza a buscar en los 1000 mas frecuentes
        auto [id_top, d_top] = nearest_in_subset(topFreqK);//busca el mas cercano en los 1000 mas frecuentes
        t1 = clk::now(); //Guarda el instante en el que termina de buscar en los 1000 mas frecuentes
        auto ms_top = std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count();//tiempo que se ha tardado en lso 1000 mas frecuentes

		//Aqui imprimimos los resultados
        std::cout << "Más Cercano En Los Últimos 1000: ID = " << id_last << " | distancia (km) = " << d_last << " | tiempo (ns) = " << ms_last << "\n";
        std::cout << "Más Cercano En Los Últimos 1000 Más Frecuentes: ID = " << id_top << " | distancia (km) = " << d_top << " | tiempo (ns) = " << ms_top << "\n\n";
        omp_set_num_threads(numThreads); //se vuelve al numero de threads original
    }

    // Apartado 4a
    {
        std::cout << "----- Apartado 4a -----\n";
        const glm::dvec2 target(20.525750, -97.46000); //Este el punto objetivo
        const double radius_km = 30.0; //rango máximo de búsqueda

        size_t K = std::min<size_t>(1000, rows.size()); ///K es el minimo entre 1000 y el numero las filas
        std::vector<compute::CellType> lastK; //Vector de los ultimos 1000
        lastK.reserve(K); //Reservo capacidad
        for (size_t i = rows.size() - K; i < rows.size(); ++i) lastK.push_back(rows[i]); //Copiamso las K ultimas apariciones a lastK

        const unsigned int cores = maxThreads; //Guardamos los threads disponibles
        const unsigned int tiles = 4; // 4 bloques
        const unsigned int threads_per_tile = std::max(1u, cores / tiles); //ccuantos threads usamos por cada  tile

        auto t0 = clk::now(); //Guardamos el momento en el que se empieza
        double best_d = 1e300; //Se inicializa la mejor opcion a mucha distancia e id =0
        unsigned int best_id = 0;

        // Dividir el vector en 'tiles' segmentos y procesar cada uno con threads_per_tile hilos
        std::vector<size_t> starts; //Vector en el que guardamos el inicio de cada tile
        for (unsigned int t = 0; t < tiles; ++t) {
            starts.push_back( t * (lastK.size() / tiles) ); //Metemso el inicio de cada tile en el vector starts
        }
        starts.push_back(lastK.size()); //Añadimos el indice final para poder usar rangos

        for (unsigned int t = 0; t < tiles; ++t) { //Bucle qeu recorre cada tile
            omp_set_num_threads(threads_per_tile); //Se usan los threads calculados para cada tile
            size_t beg = starts[t];//variables de empezarr y finalizar para saber el rango actual
            size_t end = starts[t+1];

            double local_best_d = 1e300; //Inicializamos una mejro opcion local de cada tile con distancia muy grande
            unsigned int local_best_id = 0;

            #pragma omp parallel //se inicia seccion paralela
            {
                double th_best_d = 1e300; //Inicializamos una mejro opcion de cada hilo con distancia muy grande
                unsigned int th_best_id = 0;
                #pragma omp for nowait //Sin barrera
                for (long i = static_cast<long>(beg); i < static_cast<long>(end); ++i) { //se recorre solo entre el rango del tile
                    const auto& c = lastK[i]; //se obtiene la aparicion
                    double d = compute::calculateDistance(target, c.coordinates); //Se calcula su distancia al punto
                    if (d <= radius_km && d < th_best_d) { //Si la distancia es mas pequeña que el radio y que la distancia al que era el mejor
                        th_best_d = d; //Este pasa a ser la mejor opcion para este hilo
                        th_best_id = c.id;
                    }
                }
                #pragma omp critical //Seccion critica cuando termina cad threada
                {
                    if (th_best_d < local_best_d) { local_best_d = th_best_d; local_best_id = th_best_id; }
                    //Si el mejor del hilo es mejor que el que tenemos como mejor del tile, este pasa a ser el nuevo mejor del tile
                }
            }
			//Cuando terminan los hilos ya tenemos el mejor del tile
            if (local_best_d < best_d) { best_d = local_best_d; best_id = local_best_id; }
            //Si el mejor del tile es mejor que el global, este pasa a ser el nuebvo mejor global
        }

        auto t1 = clk::now();//Capturamso cuando terminan todos los tiles
        auto ms_tiles = std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count(); //Tiempo qque han tardado los tiles en buscar

        std::cout << "Resultado de los tiles: ID = " << best_id << " | distancia (km) = " << best_d
                  << " | tiempo (ns) = " << ms_tiles << " | (tiles = " << tiles
                  << " , threads/tile = " << threads_per_tile << ")\n\n";
    }

    // Apartado 4b
    {
        std::cout << "----- Apartado 4b -----\n";
        const glm::dvec2 target(20.525750, -97.46000); //Este el punto objetivo
        const double radius_km = 30.0; //rango máximo de búsqueda

        size_t K = std::min<size_t>(1000, rows.size()); ///K es el minimo entre 1000 y el numero las filas
        std::vector<compute::CellType> lastK; //Vector de los ultimos 1000
        lastK.reserve(K); //Reservo capacidad
        for (size_t i = rows.size() - K; i < rows.size(); ++i) lastK.push_back(rows[i]); //Copiamso las K ultimas apariciones a lastK

        omp_set_num_threads(numThreads); // restablecer hilos

        auto t0 = clk::now(); //Guardamos momento en el que empieza
        double best_d = 1e300; //Se inicializa la mejor opcion a mucha distancia e id =0
        unsigned int best_id = 0;

        #pragma omp parallel //Empieza la seccion paralela
        {
            double local_best_d = 1e300; //Inicializamos una mejro opcion localcon distancia muy grande
            unsigned int local_best_id = 0;

            #pragma omp for nowait //Sin barreras
            for (long i = 0; i <= static_cast<long>(lastK.size()) - 4; i += 4) { //bucle de 4 en 4
                const auto& c0 = lastK[i+0];
                const auto& c1 = lastK[i+1];
                const auto& c2 = lastK[i+2];
                const auto& c3 = lastK[i+3]; //las 4 apariciones de este bucle

                double d0 = compute::calculateDistance(target, c0.coordinates);
                double d1 = compute::calculateDistance(target, c1.coordinates);
                double d2 = compute::calculateDistance(target, c2.coordinates);
                double d3 = compute::calculateDistance(target, c3.coordinates); //Las 4 distancias al punto

                if (d0 <= radius_km && d0 < local_best_d) { local_best_d = d0; local_best_id = c0.id; }
                if (d1 <= radius_km && d1 < local_best_d) { local_best_d = d1; local_best_id = c1.id; }
                if (d2 <= radius_km && d2 < local_best_d) { local_best_d = d2; local_best_id = c2.id; }
                if (d3 <= radius_km && d3 < local_best_d) { local_best_d = d3; local_best_id = c3.id; }
                //Si alguno es mejor opcion, se convierte en la mejro opcion local
            }

            // Resto por si hay elemnetos que no han entrado (porque no era multiplo de 44 el tamaño)
            #pragma omp for nowait //Sin barreras
            for (long i = (static_cast<long>(lastK.size()) / 4) * 4; i < static_cast<long>(lastK.size()); ++i) {
                const auto& c = lastK[i];//Lso rrecorre y hace lo mismo pero de un  en uno
                double d = compute::calculateDistance(target, c.coordinates);
                if (d <= radius_km && d < local_best_d) { local_best_d = d; local_best_id = c.id; }
            }

            #pragma omp critical//Empieza seccion critica
            {
                if (local_best_d < best_d) { best_d = local_best_d; best_id = local_best_id; } //Si el local es mejor, apsa a a ser la mejor opcion
            }
        }

        auto t1 = clk::now(); //Guardamso el momento en el que termina
        auto ms_unroll = std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count(); //Tiempo que ha tardado

        std::cout << "Resultado Del Desenrrollado: ID = " << best_id << " | distancia (ns) = " << best_d
                  << " | tiempo (ns) = " << ms_unroll << "\n\n";
    }

    return 0;
}
