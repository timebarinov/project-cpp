
#include <iostream>
#include <cstdio>
#include "routemanager.h"
#include "test_runner.h"

void TestRoute() {
    RouteManager route(std::cout);
    route.ReadQuery("Stop Tolstopaltsevo: 55.611087, 37.20829");
    route.ReadQuery("Stop Marushkino: 55.595884, 37.209755");
    route.ReadQuery("Bus 256: Biryulyovo Zapadnoye > Biryusinka > Universam > Biryulyovo Tovarnaya > Biryulyovo Passazhirskaya > Biryulyovo Zapadnoye");
    route.ReadQuery("Bus 750: Tolstopaltsevo - Marushkino - Rasskazovka");
    route.ReadQuery("Stop Rasskazovka: 55.632761, 37.333324");
    route.ReadQuery("Stop Biryulyovo Zapadnoye: 55.574371, 37.6517");
    route.ReadQuery("Stop Biryusinka: 55.581065, 37.64839");
    route.ReadQuery("Stop Universam: 55.587655, 37.645687");
    route.ReadQuery("Stop Biryulyovo Tovarnaya: 55.592028, 37.653656");
    route.ReadQuery("Stop Biryulyovo Passazhirskaya: 55.580999, 37.659164");

    std::string answer;
    answer = route.GetStops("256");
    ASSERT_EQUAL(answer, "Bus 256: 6 stops on route, 5 unique stops, 4371.02 route length");

    answer = route.GetStops("750");
    ASSERT_EQUAL(answer, "Bus 750: 5 stops on route, 3 unique stops, 20939.5 route length");

    answer = route.GetStops("751");
    ASSERT_EQUAL(answer, "Bus 751: not found");
}

void TestsProcessRoute() {
    std::stringstream out;
    RouteManager route(out);

    std::stringstream in(R"(10
Stop Tolstopaltsevo: 55.611087, 37.20829
Stop Marushkino: 55.595884, 37.209755
Bus 256: Biryulyovo Zapadnoye > Biryusinka > Universam > Biryulyovo Tovarnaya > Biryulyovo Passazhirskaya > Biryulyovo Zapadnoye
Bus 750: Tolstopaltsevo - Marushkino - Rasskazovka
Stop Rasskazovka: 55.632761, 37.333324
Stop Biryulyovo Zapadnoye: 55.574371, 37.6517
Stop Biryusinka: 55.581065, 37.64839
Stop Universam: 55.587655, 37.645687
Stop Biryulyovo Tovarnaya: 55.592028, 37.653656
Stop Biryulyovo Passazhirskaya: 55.580999, 37.659164
3
Bus 256
Bus 750
Bus 751
)");

    std::string expected(R"(Bus 256: 6 stops on route, 5 unique stops, 4371.02 route length
Bus 750: 5 stops on route, 3 unique stops, 20939.5 route length
Bus 751: not found
)");

    route.ProcessRoute(in);

    ASSERT_EQUAL(expected, out.str());
};

void TestAll() {
    TestRunner tr;
    RUN_TEST(tr, TestRoute);
    RUN_TEST(tr, TestsProcessRoute);

}

int main() {
    TestAll();

    auto route = RouteManager();
    route.ProcessRoute();
    return 0;
};