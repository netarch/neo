//
// Created by Santhosh Prabhu on 1/29/20.
//

#include "choices.hpp"

size_t Choices::hash() const {

    size_t value = 0;
    for(const auto &entry:this->_choicesMap) {
        ::hash::hash_combine(value, size_t(entry.first.first));
        ::hash::hash_combine(value, size_t(entry.first.second));
        ::hash::hash_combine(value, size_t(entry.second.get_l3_node()));
    }
    return value;
}

bool Choices::equal(const Choices *other) const {
    return static_cast<const Choices *>(other)->_choicesMap == this->_choicesMap;
}

Choices *Choices::createNewObjectWithNewChoice(EqClass *ec, Node *node, FIB_IPNH choice) const {
    auto res = new Choices(*this);
    res->_choicesMap.emplace(std::make_pair(ec, node), std::move(choice));
    return res;
}

Choices *Choices::setChoice(EqClass *ec, Node *node, FIB_IPNH choice) const {
    auto newChoiceObj = this->createNewObjectWithNewChoice(ec, node, std::move(choice));
    auto [itr, insertRes] = choicesSet.insert(newChoiceObj);
    if(insertRes) {
        return newChoiceObj;
    } else {
        delete newChoiceObj;
        return *itr;
    }
}

std::optional<FIB_IPNH> Choices::getChoiceIfPresent(EqClass *ec, Node *node) const {
    auto itr = this->_choicesMap.find(std::make_pair(ec, node));
    if(itr != this->_choicesMap.end()) {
        return std::optional<FIB_IPNH>(itr->second);
    }

    return std::optional<FIB_IPNH>();
}

std::optional<FIB_IPNH> getChoiceIfPresent(State *state) {
    Choices *choicesObj;
    memcpy(&choicesObj, state->comm_state[state->comm].path_choices,
           sizeof(Choices *));

    if(choicesObj == nullptr) { //no choices made so far
        return std::optional<FIB_IPNH>();
    }

    EqClass *curEc;
    memcpy(&curEc, state->comm_state[state->comm].ec, sizeof(EqClass *));
    Node *curNode;
    memcpy(&curNode, state->comm_state[state->comm].pkt_location, sizeof(Node *));
    return choicesObj->getChoiceIfPresent(curEc, curNode);
}

void setChoice(State *state, FIB_IPNH choice) {
    Choices *choicesObj;
    memcpy(&choicesObj, state->comm_state[state->comm].path_choices,
           sizeof(Choices *));
    EqClass *curEc;
    memcpy(&curEc, state->comm_state[state->comm].ec, sizeof(EqClass *));
    Node *curNode;
    memcpy(&curNode, state->comm_state[state->comm].pkt_location, sizeof(Node *));

    bool newObj = false;
    if(choicesObj == nullptr) { //no choices made so far
        choicesObj = new Choices();
        newObj = true;
    }

    auto newChoicesObj = choicesObj->setChoice(curEc, curNode, std::move(choice));

    if(newObj) {
        delete choicesObj;
    }

    memcpy(state->comm_state[state->comm].path_choices, &newChoicesObj,
           sizeof(Choices *));

}