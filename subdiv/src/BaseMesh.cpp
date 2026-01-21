#include "BaseMesh.hpp"

#include "utils.hpp"

namespace subdiv 
{
    Face* BaseMesh::addFace(const std::vector<int>& vertIds) {
        assert(vertIds.size() >= 3);

        Face* f = new Face();
        faces.push_back(f);

        std::vector<HalfEdge*> faceEdges;
        for (size_t i = 0; i < vertIds.size(); i++) {
            HalfEdge* he = new HalfEdge();
            he->to = vertices[vertIds[i]];
            he->face = f;
            faceEdges.push_back(he);
            halfedges.push_back(he);

            if (!vertices[vertIds[i]]->outgoing) {
                vertices[vertIds[i]]->outgoing = he;
            }
        }

        int N = int(faceEdges.size());
        for (int i = 0; i < N; i++) {
            faceEdges[i]->next = faceEdges[(i + 1) % N];
            faceEdges[(i + 1) % N]->prev = faceEdges[i];
        }

        f->edge = faceEdges[0];
        return f;
    }

    void BaseMesh::setEdgeCrease(HalfEdge* e, float sharpness) {
        e->tag = (sharpness >= 1.0f) ? EdgeTag::EDGE_HARD : EdgetTag::EDGE_SEMI;
        e->sharpness = sharpness;
        if (e->twin) {
            e->twin->tag = e->tag;
            e->twin->sharpness = sharpness;
        }
    }

    Vertex* BaseMesh::splitEdge(HalfEdge* e) {
        Vertex* vNew = new Vertex();
        vNew->position = 0.5f * (e->to->position + e->prev->to->position);
        vertices.push_back(vNew);

        HalfEdge* eNew = new HalfEdge();
        eNew->to = e->to;
        e->to = vNew;
        eNew->face = e->face;
        halfedges.push_back(eNew);

        eNew->next = e->next;
        eNew->prev = e;
        e->next = eNew;
        if (eNew->next) eNew->next->prev = eNew;

        return vNew;
    }

    Vertex* BaseMesh::splitFace(Face* f) {
        glm::vec3 center(0.0f);
        int N = f->valence();
        HalfEdge* e = f->edge;
        do {
            center += e->to->position;
            e = e->next;
        } while (e != f->edge);
        center /= float(N);

        Vertex* vCenter = new Vertex();
        vCenter->position = center;
        vertices.push_back(vCenter);

        return vCenter;
    }


    void BaseMesh::rebuildConnectivity() {
        struct pair_hash {
            template <class T1, class T2>
            std::size_t operator () (const std::pair<T1*,T2*> &p) const {
                return std::hash<T1*>{}(p.first) ^ std::hash<T2*>{}(p.second);
            }
        };

        std::unordered_map<std::pair<Vertex*,Vertex*>, HalfEdge*, pair_hash> edgeMap;
        for (auto e : halfedges) {
            edgeMap[std::make_pair(e->prev->to, e->to)] = e;
        }
        for (auto e : halfedges) {
            auto twinKey = std::make_pair(e->to, e->prev->to);
            if (edgeMap.count(twinKey)) e->twin = edgeMap[twinKey];
        }
    }

} // namespace subdiv