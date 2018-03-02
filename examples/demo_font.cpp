// demo_font.cpp created on 2018-03-02, part of XCI toolkit
//
// Set WORKDIR to project root.

#include <xci/text/FontLibrary.h>
#include <xci/text/FontFace.h>
#include <xci/text/Font.h>
#include <xci/text/Text.h>

#include <SFML/Graphics.hpp>

#include <iostream>
#include <thread>
#include <mutex>

using namespace xci::text;

static const char * some_text =
        "One morning, when Gregor Samsa \n"
        "woke from troubled dreams, he found\n"
        "himself transformed in his bed into\n"
        "a horrible vermin. He lay on his\n"
        "armour-like back, and if he lifted\n"
        "his head a little he could see his\n"
        "brown belly, slightly domed and\n"
        "divided by arches into stiff sections.\n"
        "The bedding was hardly able to cover\n"
        "it and seemed ready to slide off any\n"
        "moment.";

std::mutex cout_mutex;

void thread_run(const std::string& thread_name)
{
    std::lock_guard<std::mutex> lock(cout_mutex);
    std::cout << thread_name << ": "
              << (size_t) FontLibrary::get_default_instance()->raw_handle()
              << std::endl;
}

int main()
{
    // each thread has its own static instance of FontLibrary
    std::cout << "main: "
              << (size_t) FontLibrary::get_default_instance()->raw_handle()
              << std::endl;

    std::thread a(thread_run, "thread1"), b(thread_run, "thread2");

    a.join();
    b.join();

    FontFace face;
    face.load_from_file("fonts/Share_Tech_Mono/ShareTechMono-Regular.ttf", 0);

    Font font;
    font.add_face(face);

    Text text;
    text.set_string(some_text);
    text.set_font(font);
    text.set_size(20);
    text.set_color(sf::Color::White);

    sf::Sprite sprite(font.get_texture());
    sprite.setPosition({-300, -200});

    sf::RenderWindow window(sf::VideoMode(800, 600), "SFML window");
    sf::View view;
    view.setCenter(0, 0);
    view.setSize(800, 600);
    window.setView(view);

    sf::Event event = {};
    while (window.isOpen()) {
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed)
                window.close();
        }

        window.clear();
        sf::RenderStates states;
        states.transform.translate({-100, -200});
        window.draw(text, states);
        window.draw(sprite);
        window.display();
    }
    return EXIT_SUCCESS;
}
