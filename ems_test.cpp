#include "ems.hpp"

#include <cmath>
#include <iostream>

struct Vec2 {
    float x;
    float y;

    friend std::ostream& operator<<(std::ostream& out, const Vec2& vec)
    {
        out << '(' << vec.x << ", " << vec.y << ')';
        return out;
    }
};

struct Atom {
    Vec2 position;
    float radius;
};

template <typename T1, typename T2>
struct CollisionEvent : public std::pair<T1, T2> {
    constexpr CollisionEvent(const T1& box1, const T2& box2)
        : std::pair<T1, T2>{box1, box2}
    {
    }
};

using AtomCollision = CollisionEvent<Atom, Atom>;

struct ExplosionEvent {
    float blast_force;
    Vec2 position;
};

struct WipeoutEvent {
    float destruction_rate;
};

template <typename Dispatcher>
void on_atom_collision(const AtomCollision& e, Dispatcher& d)
{
    std::cout << "Detected collision of atoms:\n\t first atom: "
              << e.first.position << "\n\tsecond atom: " << e.second.position
              << std::endl;

    const auto [aposx, aposy] = e.first.position;
    const auto [bposx, bposy] = e.second.position;
    const auto blast_force = std::powf(e.first.radius * e.second.radius, 2);
    d.send(ExplosionEvent{
        .blast_force = blast_force,
        .position = Vec2{(aposx + bposx) / 2, (aposy + bposy) / 2},
    });
    if (blast_force > 10'000) {
        d.send(WipeoutEvent{.destruction_rate = blast_force / 1e8f});
    }
}

void explode(const ExplosionEvent& e)
{
    std::cout << "Boom! Explosion at " << e.position << " with "
              << e.blast_force << " newtons of force" << std::endl;
}

struct World {
    void wipeout(const WipeoutEvent& e)
    {
        std::cout
            << "The whole humanity is being wiped out due to a large blast"
            << std::endl;
        if (e.destruction_rate > durability_rate) {
            std::cout << "Earth is destroyed as well" << std::endl;
        }
    }

    float durability_rate = 1.5f;
};

int main()
{
    using event_registry =
        std::tuple<AtomCollision, ExplosionEvent, WipeoutEvent>;
    ems::dispatcher<event_registry> dispatcher{};
    auto on_atom_collision_wrapper = [&dispatcher](auto&& e) {
        return on_atom_collision(e, dispatcher);
    };
    dispatcher.subscribe<AtomCollision>(on_atom_collision_wrapper);
    dispatcher.subscribe<ExplosionEvent>(&explode);
    Atom atom1{.position = {130, 150}, .radius = 62.5};
    Atom atom2{.position = {120, 160}, .radius = 200};
    World w;
    dispatcher.subscribe<WipeoutEvent, &World::wipeout>(w);
    dispatcher.send(AtomCollision{atom1, atom2});
}
