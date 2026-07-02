#pragma once

#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <set>
#include <algorithm>

struct CsvRow {
    unsigned int id;
    std::string name;
    std::string continent;
    glm::dvec2 coordinates;
};

class CSVLoader {
public:
    static std::vector<CsvRow> load(const std::string &path) {
        std::vector<CsvRow> rows;
        std::ifstream file(path);
        if (!file.is_open()) {
            std::cerr << "Error leyendo CSV: No se pudo abrir el archivo CSV\n";
            return rows;
        }

        std::string line;
        // Leer cabecera
        if (!std::getline(file, line)) {
            return rows;
        }

        // Conjunto de continentes válidos
        static const std::set<std::string> VALID_CONTINENTS = {
            "America", "Europe", "Africa", "Asia", "Pacific"
        };

        while (std::getline(file, line)) {
            if (line.empty()) continue;

            std::stringstream ss(line);
            std::string field;

            CsvRow row{};

            // id
            if (!std::getline(ss, field, ',')) continue;
            row.id = static_cast<unsigned int>(std::stoul(field));

            // name
            if (!std::getline(ss, field, ',')) continue;
            row.name = field;

            // lat
            if (!std::getline(ss, field, ',')) continue;
            double lat = std::stod(field);

            // lon
            if (!std::getline(ss, field, ',')) continue;
            double lon = std::stod(field);

            // continent bruto
            if (!std::getline(ss, field, ',')) continue;
            std::string cont = field;

            auto pos = cont.find('/');
            if (pos != std::string::npos) {
                cont = cont.substr(0, pos);
            }

            auto trim = [](std::string &s) {
                s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) {
                    return !std::isspace(ch);
                }));
                s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) {
                    return !std::isspace(ch);
                }).base(), s.end());
            };
            trim(cont);

            // Filtrar solo los 5 continentes del enunciado
            if (!VALID_CONTINENTS.count(cont)) {
                continue; // ignorar esta fila
            }

            row.continent = cont;
            row.coordinates = glm::dvec2(lat, lon);

            rows.push_back(row);
        }

        return rows;
    }
};

