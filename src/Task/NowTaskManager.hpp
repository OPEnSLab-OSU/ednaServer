#pragma once
#include <KPFoundation.hpp>
#include <KPSubject.hpp>
#include <KPDataStoreInterface.hpp>

#include <Task/NowTask.hpp>
#include <Task/NowTaskObserver.hpp>
#include <Task/TaskObserver.hpp>
#include <Application/Config.hpp>

#include <vector>
#include "SD.h"

class NowTaskManager : public KPComponent,
                    public JsonEncodable,
                    public Printable,
                    public KPSubject<NowTaskObserver> {

public:
    const char * taskFolder = nullptr;
    NowTask task;

    NowTaskManager() : KPComponent("NowTaskManager") {}

    void init(Config & config) {
        taskFolder = config.taskFolder;
    }

    int generateTaskId() const {
        return random(1, RAND_MAX);
    }
/*
    Task createTask() {
        const auto timenow = now();
        Task task;
        task.id        = generateTaskId();
        task.createdAt = timenow;
        task.schedule  = timenow;
        return task;
    }


    bool advanceTask(int id) {
        if (!findTask(id)) {
            return false;
        }

        auto & task = tasks[id];
        println(GREEN("Task Time betwen: "), task.timeBetween);
        task.schedule = now() + std::max(task.timeBetween, 5);
        if (++task.valveOffsetStart >= task.getNumberOfValves()) {
            return markTaskAsCompleted(id);
        }

        return true;
    }
*/
    bool setTaskStatus(int id, TaskStatus status) {
        task.status = status;
        updateObservers(&NowTaskObserver::taskDidUpdate, task);
        return true;
    }

    int numberOfActiveTasks() const {
        if(task.status == TaskStatus::active){
            return 1;
        }
        return 0;
    }

    /*
    bool markTaskAsCompleted(int id) {
        task.valves.clear();
        if (task.deleteOnCompletion) {
            println("DELETED: ", id);
            deleteTask(id);
        } else {
            task.status = TaskStatus::completed;
            updateObservers(&TaskObserver::taskDidUpdate, task);
        }

        return true;
    }

    bool findTask(int id) const {
        return tasks.find(id) != tasks.end();
    }

    bool deleteTask(int id) {
        if (tasks.erase(id)) {
            updateObservers(&TaskObserver::taskDidDelete, id);
            return true;
        }

        return false;
    }

    int deleteIf(std::function<bool(const Task &)> predicate) {
        int oldSize = tasks.size();
        for (auto it = tasks.begin(); it != tasks.end();) {
            if (predicate(it->second)) {
                auto id = it->first;
                it      = tasks.erase(it);
                updateObservers(&TaskObserver::taskDidDelete, id);
            } else {
                it++;
            }
        }

        return oldSize - tasks.size();
    }
    */

    /** ────────────────────────────────────────────────────────────────────────────
     *  @brief Load all tasks object from the specified directory in the SD card
     *
     *  @param _dir Path to tasks directory (default=~/tasks)
     *  ──────────────────────────────────────────────────────────────────────────── */
    void loadTasksFromDirectory(const char * _dir = nullptr) {
        const char * dir = _dir ? _dir : taskFolder;

        JsonFileLoader loader;
        loader.createDirectoryIfNeeded(dir);

        // Load task index file and get the number of tasks
        auto start = millis();
        KPStringBuilder<32> filepath(dir, "/NowTask.js");
        loader.load(filepath, task);

        println(GREEN("Task Manager"), " finished reading in ", millis() - start, " ms\n");
        // updateObservers(&TaskObserver::taskCollectionDidUpdate, tasks.begin());
    }


    /** ────────────────────────────────────────────────────────────────────────────
     *  @brief Write task array to SD directory
     *
     *  @param _dir Path to tasks directory (default=~/tasks)
     *  ──────────────────────────────────────────────────────────────────────────── */
    void writeToDirectory(const char * _dir = nullptr) {
        const char * dir = _dir ? _dir : taskFolder;

        JsonFileLoader loader;
        loader.createDirectoryIfNeeded(dir);

        println("Writing Now Task");

        KPStringBuilder<32> filename("NowTask.js");
        KPStringBuilder<64> filepath(dir, "/", filename);
        loader.save(filepath, task);
    }

#pragma region JSONENCODABLE
    static const char * encoderName() {
        return "NowTaskManager";
    }

    static constexpr size_t encodingSize() {
        return NowTask::encodingSize() * 5;
    }

    bool encodeJSON(const JsonVariant & dst) const override {
        JsonVariant obj = dst.createNestedObject();
        if (!task.encodeJSON(obj)) {
            return false;
        }
        return true;
    }
#pragma endregion
#pragma region PRINTABLE
    size_t printTo(Print & p) const {
        size_t charWritten = 0;
        charWritten += p.println("[");
        charWritten += p.print(task);
        /*for (const auto & kv : tasks) {
            charWritten += p.print(kv.second);
            charWritten += p.println(",");
        }*/

        return charWritten + p.println("]");
    }
#pragma endregion
};
