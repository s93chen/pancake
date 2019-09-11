#include <string>
#include <vector>
#include <cerrno>
#include <cstring>
#include <fstream>
#include <sstream>
#include <iostream>
#include <stdexcept>
#include <algorithm>
#include <unordered_map>
#include <unordered_set>

#include <wait.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>


struct Target {
  std::vector<std::string> neighbours;
  std::vector<std::string> commands;
};

typedef std::unordered_map<std::string, Target> Graph;

template <typename T>
std::ostream& operator<<(
  std::ostream& out,
  const std::vector<T> &item)
{
  out << "[ ";
  for (const T& s : item)
  out << s << " ";
  return out << "]";
}

std::ostream& operator<<(
  std::ostream& out,
  const Graph &graph)
{
  for (auto i=graph.begin(); i!=graph.end(); i++) {
    out << "* Target: " << i->first << "\n";
    out << "  - Dependencies: ";
    out << (i->second).neighbours << "\n";
    out << "  - Commands: ";
    out << (i->second).commands << "\n";
  }
  return out;
}

void readLines(
  const std::string &path,
  std::vector<std::string> &lines)
{
  std::ifstream file(path);
  if (!file.is_open())
    throw std::runtime_error("Pancake does not exist");

  std::string line;
  while (std::getline(file, line))
    lines.emplace_back(line);

  if (file.bad())
    throw std::runtime_error("Error while reading file");
}

bool isTarget(std::string &line)
{
  return line.find(":") < line.find('\t');
}

bool isCommand(std::string &line)
{
  return line.find('\t') == 0;
}

int trimLine(
  std::string &line,
  const std::string &trimChar)
{
  line.erase(0, line.find_first_not_of(trimChar));
  line.erase(line.find_last_not_of(trimChar) + 1, line.size());
  return line.size();
}

void buildGraph(std::vector<std::string> &lines, Graph &graph)
{
  size_t split, pos, start;
  std::string depend, token;
  std::string target = "";

  for (int i=0; i<lines.size(); i++) {
    std::string &cur = lines.at(i);

    if (isTarget(cur) && (trimLine(cur, " \t\n") > 0)) {
      split = cur.find(":");
      target = cur.substr(0, split);

      // Square bracket operator
      // If key does not exist, its value initializes
      // a value object (no args constructor)
      graph[target];

      depend = cur.substr(split+1, cur.size()-split-1);
      trimLine(depend, " ");

      start = 0;
      while ((pos = depend.find(" ", start)) != std::string::npos) {
        token = depend.substr(start, pos-start);
        graph[target].neighbours.emplace_back(token);
        start = pos + 1;
      }
      if (start < depend.size()) {
        graph[target].neighbours.emplace_back(
            depend.substr(start, depend.size()-start)
        );
      }
    }
    else if (isCommand(cur)) {
      if (graph.find(target) != graph.end()) {
        if (trimLine(cur, " \t\n") > 0)
          graph[target].commands.emplace_back(cur);
      }
      else
        throw std::runtime_error("No valid target");
    }
  }
}

void topologicalSort(
  const std::string &node, const Graph &graph,
  std::unordered_set<std::string> &visited,
  std::vector<std::string> &sortedNodes)
{
  if (graph.find(node) == graph.end())
    throw std::runtime_error("Invalid node. Ignored.");

  if (visited.find(node) != visited.end())
    return;

  visited.insert(node);
  for (int i=0; i<graph.at(node).neighbours.size(); i++) {
    topologicalSort(
      graph.at(node).neighbours.at(i), graph,
      visited, sortedNodes
    );
  }
  sortedNodes.push_back(node);
}

void topologicalSort(
  Graph &graph,
  std::vector<std::string> &sortedNodes)
{
  std::unordered_set<std::string> visited;
  for (auto it=graph.begin(); it!=graph.end(); it++)
    topologicalSort(it->first, graph, visited, sortedNodes);
}

int runCommand(const std::string &cmd)
{
  int status;
  pid_t pid = fork();

  char *const argv[] = {
    (char * const) "bash", (char * const) "-c",
    (char * const) cmd.c_str(), NULL
  };

  if (pid == -1)
    return -1;

  if (pid == 0) {
    execvp("bash", argv);
    throw std::runtime_error(std::strerror(errno));
  }
  if (waitpid(pid, &status, 0) == -1)
    return -1;

  return status;
}

void runCommands(
  const Graph &graph,
  const std::vector<std::string> &sortedNodes)
{
  int status;

  for (std::string target : sortedNodes){
    for (std::string cmd : graph.at(target).commands) {
      std::cout << cmd << "\n";
      status = runCommand(cmd);

      if (status < 0)
        throw std::runtime_error(std::strerror(errno));
      if (!WIFEXITED(status) || WEXITSTATUS(status))
        throw std::runtime_error("Child failed.");
    }
  }
}

int main(int argc, char *argv[])
{
  if (argc != 2)
    throw std::runtime_error("Expect 1 pancake file");

  Graph graph;
  std::vector<std::string> lines;
  std::vector<std::string> sortedTargets;

  readLines(argv[1], lines);
  buildGraph(lines, graph);
  topologicalSort(graph, sortedTargets);

  std::cout << graph << "\n";
  std::cout << sortedTargets << "\n";

  runCommands(graph, sortedTargets);
}
