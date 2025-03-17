#include <Wt/Dbo/Dbo.h>
#include <Wt/Dbo/backend/Postgres.h>
#include <iostream>
#include <string>

namespace dbo = Wt::Dbo;

// Определяем ORM-классы
class Publisher;
class Book;
class Shop;
class Stock;
class Sale;

class Publisher {
public:
    std::string name;
    dbo::collection<dbo::ptr<Book>> books;

    template<class Action>
    void persist(Action& a) {
        dbo::field(a, name, "name");
        dbo::hasMany(a, books, dbo::ManyToOne, "publisher");
    }
};

class Book {
public:
    std::string title;
    dbo::ptr<Publisher> publisher;
    dbo::collection<dbo::ptr<Stock>> stocks;

    template<class Action>
    void persist(Action& a) {
        dbo::field(a, title, "title");
        dbo::belongsTo(a, publisher, "publisher");
        dbo::hasMany(a, stocks, dbo::ManyToOne, "book");
    }
};

class Shop {
public:
    std::string name;
    dbo::collection<dbo::ptr<Stock>> stocks;

    template<class Action>
    void persist(Action& a) {
        dbo::field(a, name, "name");
        dbo::hasMany(a, stocks, dbo::ManyToOne, "shop");
    }
};

class Stock {
public:
    int count;
    dbo::ptr<Book> book;
    dbo::ptr<Shop> shop;
    dbo::collection<dbo::ptr<Sale>> sales;

    template<class Action>
    void persist(Action& a) {
        dbo::field(a, count, "count");
        dbo::belongsTo(a, book, "book");
        dbo::belongsTo(a, shop, "shop");
        dbo::hasMany(a, sales, dbo::ManyToOne, "stock");
    }
};

class Sale {
public:
    double price;
    std::string date_sale;
    int count;
    dbo::ptr<Stock> stock;

    template<class Action>
    void persist(Action& a) {
        dbo::field(a, price, "price");
        dbo::field(a, date_sale, "date_sale");
        dbo::field(a, count, "count");
        dbo::belongsTo(a, stock, "stock");
    }
};

int main() {
    try {
        // Подключение к PostgreSQL
        auto connection = std::make_unique<dbo::backend::Postgres>(
            "host=127.0.0.1 port=5432 dbname= user= password="
        );
        dbo::Session session;
        session.setConnection(std::move(connection));

        // Регистрация ORM-классов
        session.mapClass<Publisher>("publisher");
        session.mapClass<Book>("book");
        session.mapClass<Shop>("shop");
        session.mapClass<Stock>("stock");
        session.mapClass<Sale>("sale");

        // Создание таблиц (если они не существуют)
        {
            dbo::Transaction transaction(session);
            session.createTables();
            transaction.commit();
        }

        // Заполнение тестовыми данными
        {
            dbo::Transaction transaction(session);

            auto publisher1 = session.add(std::make_unique<Publisher>());
            publisher1.modify()->name = "Publisher1";

            auto publisher2 = session.add(std::make_unique<Publisher>());
            publisher2.modify()->name = "Publisher2";

            auto book1 = session.add(std::make_unique<Book>());
            book1.modify()->title = "Book1";
            book1.modify()->publisher = publisher1;

            auto book2 = session.add(std::make_unique<Book>());
            book2.modify()->title = "Book2";
            book2.modify()->publisher = publisher2;

            auto shop1 = session.add(std::make_unique<Shop>());
            shop1.modify()->name = "Shop1";

            auto shop2 = session.add(std::make_unique<Shop>());
            shop2.modify()->name = "Shop2";

            auto stock1 = session.add(std::make_unique<Stock>());
            stock1.modify()->count = 10;
            stock1.modify()->book = book1;
            stock1.modify()->shop = shop1;

            auto stock2 = session.add(std::make_unique<Stock>());
            stock2.modify()->count = 5;
            stock2.modify()->book = book2;
            stock2.modify()->shop = shop2;

            auto sale1 = session.add(std::make_unique<Sale>());
            sale1.modify()->price = 19.99;
            sale1.modify()->date_sale = "2023-01-01";
            sale1.modify()->count = 2;
            sale1.modify()->stock = stock1;

            auto sale2 = session.add(std::make_unique<Sale>());
            sale2.modify()->price = 29.99;
            sale2.modify()->date_sale = "2023-01-02";
            sale2.modify()->count = 1;
            sale2.modify()->stock = stock2;

            transaction.commit();
        }

        // Ввод имени или идентификатора издателя
        std::string publisher_input;
        std::cout << "Enter publisher name or ID: ";
        std::cin >> publisher_input;

        // Поиск издателя
        dbo::Transaction transaction(session);
        dbo::ptr<Publisher> deal_publisher;

        try {
            int publisher_id = std::stoi(publisher_input);
            deal_publisher = session.find<Publisher>().where("id = ?").bind(publisher_id);
        }
        catch (const std::invalid_argument&) {
            deal_publisher = session.find<Publisher>().where("name = ?").bind(publisher_input);
        }

        if (!deal_publisher) {
            std::cerr << "Publisher not found!" << std::endl;
            return 1;
        }

        // Поиск магазинов, продающих книги целевого издателя
        std::set<std::string> shop_names; // Используем set для уникальности магазинов

        for (const auto& book : deal_publisher->books) {
            for (const auto& stock : book->stocks) {
                shop_names.insert(stock->shop->name);
            }
        }

        // Вывод результатов
        std::cout << "Shops selling books by publisher " << publisher_input << ":" << std::endl;
        for (const auto& shop_name : shop_names) {
            std::cout << shop_name << std::endl;
        }

        transaction.commit();
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
