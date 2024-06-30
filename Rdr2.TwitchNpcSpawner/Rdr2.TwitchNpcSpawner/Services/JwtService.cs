namespace Rdr2.TwitchNpcSpawner.Services;

using Rdr2.TwitchNpcSpawner.Entities;
using Microsoft.EntityFrameworkCore;
using Microsoft.IdentityModel.Tokens;
using System.IdentityModel.Tokens.Jwt;
using System.Security.Claims;
using System.Text;

public class JwtService(IConfiguration _conf, MySqlContext _db)
{
    public string CreateTokenForUser(UserEntity user)
    {
        var jwtSSK = new SymmetricSecurityKey(Encoding.UTF8.GetBytes(_conf["Jwt:Secret"] ?? "HelloWorldWhyIDon'tHaveSecretInMyAppsettings.json??"));
        var jwtHandler = new JwtSecurityTokenHandler();
        var jwtDescript = new SecurityTokenDescriptor
        {
            Subject = new ClaimsIdentity(new[]
            {
                    new Claim(ClaimTypes.NameIdentifier, user.Id.ToString()),
                    new Claim(ClaimTypes.Name, user.TwitchId)
                }),
            Expires = DateTime.UtcNow.AddDays(120),
            Issuer = _conf["Jwt:Issuer"],
            Audience = _conf["Jwt:Audience"],
            SigningCredentials = new SigningCredentials(jwtSSK, SecurityAlgorithms.HmacSha256Signature)
        };

        var jwt = jwtHandler.CreateToken(jwtDescript);
        return jwtHandler.WriteToken(jwt);
    }

    public async Task<UserEntity?> GetUser(string jwt)
    {
        var jwtSSK = new SymmetricSecurityKey(Encoding.UTF8.GetBytes(_conf["Jwt:Secret"] ?? "HelloWorldWhyIDon'tHaveSecretInMyAppsettings.json??"));
        var jwtHandler = new JwtSecurityTokenHandler();
        var jwtParams = new TokenValidationParameters
        {
            ValidateIssuerSigningKey = true,
            ValidateIssuer = true,
            ValidateAudience = true,
            ValidIssuer = _conf["Jwt:Issuer"],
            ValidAudience = _conf["Jwt:Audience"],
            IssuerSigningKey = jwtSSK
        };

        try
        {
            var claimsPrincipal = jwtHandler.ValidateToken(jwt, jwtParams, out var token);

            var userIdClaim = claimsPrincipal.Claims.FirstOrDefault(x => x.Type == ClaimTypes.NameIdentifier);
            if (userIdClaim is null)
                return null;
            var userIdStr = userIdClaim.Value;
            if (!int.TryParse(userIdStr, out int userId))
                return null;

            return await _db.Users.FirstOrDefaultAsync(x => x.Id == userId);
        }
        catch
        {
            return null;
        }
    }
}
