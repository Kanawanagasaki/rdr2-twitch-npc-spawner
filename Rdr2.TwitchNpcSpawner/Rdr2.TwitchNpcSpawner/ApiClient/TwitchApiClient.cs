namespace Rdr2.TwitchNpcSpawner.ApiClient;

using Rdr2.TwitchNpcSpawner.Client.Shared;
using Rdr2.TwitchNpcSpawner.Data;
using Rdr2.TwitchNpcSpawner.Entities;

public class TwitchApiClient(HttpClient _http)
{
    public Task<TwitchApiResponse<TwitchUser>?> GetMe(UserEntity user)
        => Get<TwitchUser>
        (
            user,
            "helix/users?id=" + user.TwitchId
        );

    public Task<TwitchApiResponse<TwitchCustomReward>?> GetCustomRewardsList(UserEntity user)
        => Get<TwitchCustomReward>
        (
            user,
            $"helix/channel_points/custom_rewards?broadcaster_id={user.TwitchId}&only_manageable_rewards=true"
        );

    public Task<TwitchApiResponse<TwitchCustomReward>?> CreateCustomReward(UserEntity user, CustomRewardReq req)
        => Post<TwitchCustomReward>
        (
            user,
            $"helix/channel_points/custom_rewards?broadcaster_id=" + user.TwitchId,
            req
        );

    public Task<TwitchApiResponse<TwitchCustomReward>?> UpdateCustomReward(UserEntity user, string rewardId, CustomRewardReq req)
        => Patch<TwitchCustomReward>
        (
            user,
            $"helix/channel_points/custom_rewards?broadcaster_id={user.TwitchId}&id={rewardId}",
            req
        );

    public Task<int> DeleteCustomReward(UserEntity user, string rewardId)
        => Delete(user, $"helix/channel_points/custom_rewards?broadcaster_id={user.TwitchId}&id={rewardId}");

    public Task<TwitchApiResponse<Redemption>?> GetRedemption(UserEntity user, string rewardId, string redemptionId)
       => Get<Redemption>
       (
           user,
           $"helix/channel_points/custom_rewards/redemptions?broadcaster_id={user.TwitchId}&reward_id={rewardId}&status=UNFULFILLED&id={redemptionId}"
       );

    public Task<TwitchApiResponse<Redemption>?> GetFirstUnfulfilledRedemption(UserEntity user, string rewardId)
        => Get<Redemption>
        (
            user,
            $"helix/channel_points/custom_rewards/redemptions?broadcaster_id={user.TwitchId}&reward_id={rewardId}&status=UNFULFILLED&first=1"
        );

    public Task<TwitchApiResponse<Redemption>?> UpdateRedemptionStatus(UserEntity user, string rewardId, string redemptionId, ERedemptionStatus status)
        => Patch<Redemption>
        (
            user,
            $"helix/channel_points/custom_rewards/redemptions?broadcaster_id={user.TwitchId}&reward_id={rewardId}&id={redemptionId}",
            new
            {
                status = status switch
                {
                    ERedemptionStatus.FULFILLED => "FULFILLED",
                    ERedemptionStatus.UNFULFILLED => "UNFULFILLED",
                    ERedemptionStatus.CANCELED => "CANCELED",
                    _ => "Bruh, wtf, how this happened?"
                }
            }
        );

    private async Task<TwitchApiResponse<T>?> Get<T>(UserEntity user, string uri) where T : class
    {
        using var request = new HttpRequestMessage(HttpMethod.Get, uri);
        request.Headers.Add("Authorization", "Bearer " + user.AccessToken);
        using var response = await _http.SendAsync(request);
        if (!response.IsSuccessStatusCode)
            return new() { StatusCode = (int)response.StatusCode };

        var res = await response.Content.ReadFromJsonAsync<TwitchApiResponse<T>>();
        if (res is null)
            return null;

        res.StatusCode = (int)response.StatusCode;
        return res;
    }

    private async Task<TwitchApiResponse<T>?> Post<T>(UserEntity user, string uri, object json) where T : class
    {
        using var request = new HttpRequestMessage(HttpMethod.Post, uri);
        request.Headers.Add("Authorization", "Bearer " + user.AccessToken);
        request.Content = JsonContent.Create(json);
        using var response = await _http.SendAsync(request);
        if (!response.IsSuccessStatusCode)
            return new() { StatusCode = (int)response.StatusCode };

        var res = await response.Content.ReadFromJsonAsync<TwitchApiResponse<T>>();
        if (res is null)
            return null;

        res.StatusCode = (int)response.StatusCode;
        return res;
    }

    private async Task<TwitchApiResponse<T>?> Patch<T>(UserEntity user, string uri, object json) where T : class
    {
        using var request = new HttpRequestMessage(HttpMethod.Patch, uri);
        request.Headers.Add("Authorization", "Bearer " + user.AccessToken);
        request.Content = JsonContent.Create(json);
        using var response = await _http.SendAsync(request);
        if (!response.IsSuccessStatusCode)
            return new() { StatusCode = (int)response.StatusCode };

        var res = await response.Content.ReadFromJsonAsync<TwitchApiResponse<T>>();
        if (res is null)
            return null;

        res.StatusCode = (int)response.StatusCode;
        return res;
    }

    private async Task<int> Delete(UserEntity user, string uri)
    {
        using var request = new HttpRequestMessage(HttpMethod.Delete, uri);
        request.Headers.Add("Authorization", "Bearer " + user.AccessToken);
        using var response = await _http.SendAsync(request);
        return (int)response.StatusCode;
    }

    public enum ERedemptionStatus
    {
        FULFILLED, UNFULFILLED, CANCELED
    }
}
