#pragma once
#include <fstream>
#include <string>
#include <vector>

namespace pico {
    class pdf {
    private:
        std::vector<std::string> objects;
        int obj_count = 0;
        std::vector<int> xref;

        int add_object(const std::string &obj) {
            objects.push_back(obj);
            return ++obj_count;
        }

    public:
        pdf() {}

        int add_page() {
            std::string page =
                "<< /Type /Page /Parent 1 0 R /MediaBox [0 0 595 842] /Contents 0 0 R >>";
            return add_object(page);
        }

        int add_text_object(const std::string &text) {
            std::string stream =
                "<< /Length " + std::to_string(text.size()+20) + " >>\n"
                "stream\n"
                "BT /F1 12 Tf 50 780 Td (" + text + ") Tj ET\n"
                "endstream";
            return add_object(stream);
        }

        void save(const std::string &filename) {
            std::ofstream f(filename, std::ios::binary);
            if (!f) return;

            f << "%PDF-1.4\n";

            xref.clear();
            xref.push_back(0);

            // Write objects
            int offset = 9;
            for (int i = 0; i < objects.size(); i++) {
                xref.push_back(offset);
                std::string head = std::to_string(i + 1) + " 0 obj\n";
                f << head;
                offset += head.size();

                f << objects[i] << "\nendobj\n";
                offset += objects[i].size() + 7;
            }

            // Write xref
            int startxref = offset;
            f << "xref\n0 " << (objects.size() + 1) << "\n";
            for (int off : xref) {
                f << (off < 0 ? 0 : off);
                f << " 00000 n \n";
            }

            // Trailer
            f <<
                "trailer\n<< /Size " << (objects.size()+1) << " /Root 1 0 R >>\n"
                "startxref\n" << startxref << "\n%%EOF";
        }
    };
}
