#pragma once

#include <filesystem>
#include <optional>
#include <string_view>
#include <utility>

#include <cassert>

#include <git2.h>

namespace git2
{
struct environment
{
    environment(environment&&) = delete;

    environment()
    {
        git_libgit2_init();
    }

    ~environment()
    {
        git_libgit2_shutdown();
    }
};

class reference
{
private:
    git_reference* impl_{ nullptr };

public:
    reference() = default;

    explicit reference(git_reference* impl)
        : impl_(impl)
    {
    }

    reference(reference&& other)
        : impl_(std::exchange(other.impl_, nullptr))
    {
    }

    reference& operator=(reference&& other)
    {
        if (this != &other)
        {
            close();

            impl_ = std::exchange(other.impl_, nullptr);
        }

        return *this;
    }

    char const* shorthand_cstr()
    {
        assert(impl_);
        return git_reference_shorthand(impl_);
    }

    ~reference()
    {
        close();
    }

private:
    void close()
    {
        if (impl_)
            git_reference_free(impl_);
    }
};

class repository
{
private:
    git_repository* impl_{ nullptr };

public:
    repository() = default;

    repository(repository&& other)
        : impl_(std::exchange(other.impl_, nullptr))
    {
    }

    repository(git_repository* impl)
        : impl_(impl)
    {
    }

    repository& operator=(repository&& other)
    {
        if (this != &other)
        {
            close();

            impl_ = std::exchange(other.impl_, nullptr);
        }

        return *this;
    }

    static std::optional<repository> try_open(environment& env, std::filesystem::path const& path)
    {
        return try_open(env, path.c_str());
    }

    static std::optional<repository> try_open(environment& env, char const* path)
    {
        git_repository* impl;

        auto const rc = git_repository_open(&impl, path);
        if (rc)
            return std::nullopt;

        //     auto r = repo{ impl };
        // auto or = std::optional{std::move(r)};
        // return or;

        return repository{ impl };
    }

    ~repository()
    {
        close();
    }

    std::optional<reference> try_get_head()
    {
        git_reference* head;
        if (git_repository_head(&head, impl_))
            return std::nullopt;

        return reference{ head };
    }

private:
    void close()
    {
        if (impl_)
            git_repository_free(impl_);
    }
};
} // namespace git2
