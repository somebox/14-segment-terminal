#pragma once
#ifndef WORDS_H
#define WORDS_H
#include "Arduino.h"

std::vector<std::string> nouns = {
    "algorithm", "bandwidth", "cache", "database", "encryption",
    "firewall", "gigabyte", "hypervisor", "interface", "kernel",
    "latency", "middleware", "nanotechnology", "quantum", "repository",
    "server", "topology", "virtualization", "workflow", "xenon"
};

std::vector<std::string> verbs = {
    "authenticate", "compile", "debug", "deploy", "encrypt",
    "initialize", "migrate", "optimize", "parse", "render",
    "synchronize", "tokenize", "virtualize", "compute", "transmit",
    "instantiate", "refactor", "replicate", "simulate", "execute",
    "crash", "slow down", "disrupt", "corrupt", "erase", "expose",
    "generate", "hallicinate"
};

std::vector<std::string> adjectives = {
    "asynchronous", "binary", "cloud-based", "distributed", "encrypted",
    "fault-tolerant", "gigantic", "high-performance", "iterative", "lightweight",
    "modular", "nonlinear", "quantum-resistant", "scalable", "transparent",
    "ubiquitous", "virtualized", "wireless", "xenophobic", "yielding"
};

std::vector<std::string> properNouns = {
    "Oracle", "Alan Turing", "Bill Gates", "Microsoft", "Open AI",
    "Elon Musk", "Intel", "CrowdStrike", "Linux", "Linus Torvalds",
    "Mark Zuckerberg", "Google", "Apple", "Mr. Robot", "AWS",
    "Docker", "AI", "Kubernetes", "Satya Nadella", "Raspberry PI"
};

std::vector<std::string> pronouns = {
    "somehow", "you", "everyone", "it", "someone",
    "we", "they", "me", "sometimes", "rarely",
    "they", "all", "who", "that", "frequently"
    "her", "is not", "always", "never", "own"
};

std::vector<std::string> adverbs = {
    "asynchronously", "concurrently", "deterministically", "efficiently", "heuristically",
    "iteratively", "logarithmically", "modularly", "nonlinearly", "optimally",
    "parallelly", "quickly", "recursively", "securely", "synchronously",
    "transparently", "ubiquitously", "virtually", "wirelessly", "yieldingly"
};

std::vector<std::string> verbPhrases = {
    "consists of", "depends on", "includes", "results in", "is composed of",
    "is based on", "belongs to", "applies to", "deals with", "relies on", "could be",
    "focuses on", "is related to", "interacts with", "is concerned with", "is connected to",
    "leads to", "is part of", "is associated with", "might be", "will often"
};

#endif