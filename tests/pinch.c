#include <assert.h>
#include "../src/pinch.c"

static bool sample(Pinch *pinch, int count, float radius, float shift, double time) {
    const TouchPoint offsets[] = {{-1,0},{-0.6f,0.8f},{0.6f,0.8f},{1,0},{0,-1}};
    Contact contacts[5] = {0};
    for (int i = 0; i < count; i++) {
        contacts[i].id = i;
        contacts[i].state = 4;
        contacts[i].position.x = 0.5f + shift + offsets[i].x * radius;
        contacts[i].position.y = 0.5f + offsets[i].y * radius;
    }
    return detect_pinch(pinch, contacts, count, time);
}

int main(void) {
    Pinch pinch = {0};
    assert(!sample(&pinch, 4, 0.27f, 0, 1));
    assert(!sample(&pinch, 4, 0.11f, 0, 1.25));
    assert(!sample(&pinch, 5, 0.22f, 0, 2));
    assert(!sample(&pinch, 5, 0.22f, 0.14f, 2.25));
    assert(!sample(&pinch, 0, 0, 0, 2.5));
    assert(!sample(&pinch, 5, 0.15f, 0, 3));
    assert(!sample(&pinch, 5, 0.27f, 0, 3.2));
    assert(sample(&pinch, 5, 0.16f, 0, 3.4));
    assert(!sample(&pinch, 5, 0.12f, 0, 3.5));
    assert(!sample(&pinch, 4, 0.12f, 0, 3.6));
    assert(!sample(&pinch, 5, 0.26f, 0, 3.7));
    assert(!sample(&pinch, 5, 0.13f, 0, 3.9));
    assert(!sample(&pinch, 0, 0, 0, 4));
    assert(!sample(&pinch, 5, 0.26f, 0, 5));
    assert(sample(&pinch, 5, 0.17f, 0, 5.25));
}
